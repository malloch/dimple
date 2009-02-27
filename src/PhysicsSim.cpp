// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#include "dimple.h"
#include "PhysicsSim.h"

bool PhysicsPrismFactory::create(const char *name, float x, float y, float z)
{
    OscPrismODE *obj = new OscPrismODE(simulation()->odeWorld(),
                                         simulation()->odeSpace(),
                                         name, m_parent);

    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->m_position.set(x, y, z);

    return true;
}

bool PhysicsSphereFactory::create(const char *name, float x, float y, float z)
{
    OscSphereODE *obj = new OscSphereODE(simulation()->odeWorld(),
                                         simulation()->odeSpace(),
                                         name, m_parent);

    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->m_position.set(x, y, z);

    return true;
}

bool PhysicsHingeFactory::create(const char *name, OscObject *object1, OscObject *object2,
                                 double x, double y, double z, double ax, double ay, double az)
{
    OscHinge *cons=NULL;
    cons = new OscHingeODE(simulation()->odeWorld(),
                           simulation()->odeSpace(),
                           name, m_parent, object1, object2,
                           x, y, z, ax, ay, az);

    cons->m_response->traceOn();

    if (cons)
        return simulation()->add_constraint(*cons);
}

bool PhysicsHinge2Factory::create(const char *name, OscObject *object1,
                                  OscObject *object2, double x,
                                  double y, double z, double a1x,
                                  double a1y, double a1z, double a2x,
                                  double a2y, double a2z)
{
    OscHinge2 *cons=NULL;
    cons = new OscHinge2ODE(simulation()->odeWorld(),
                            simulation()->odeSpace(),
                            name, m_parent, object1, object2,
                            x, y, z, a1x, a1y, a1z, a2x, a2y, a2z);

    if (cons)
        return simulation()->add_constraint(*cons);
}

bool PhysicsFixedFactory::create(const char *name, OscObject *object1, OscObject *object2)
{
    OscFixed *cons=NULL;
    cons = new OscFixedODE(simulation()->odeWorld(),
                           simulation()->odeSpace(),
                           name, m_parent, object1, object2);

    if (cons)
        return simulation()->add_constraint(*cons);
}

bool PhysicsBallJointFactory::create(const char *name, OscObject *object1,
                                     OscObject *object2, double x, double y,
                                     double z)
{
    OscBallJoint *cons=NULL;
    cons = new OscBallJointODE(simulation()->odeWorld(),
                               simulation()->odeSpace(),
                               name, m_parent, object1, object2,
                               x, y, z);

    if (cons)
        return simulation()->add_constraint(*cons);
}

bool PhysicsSlideFactory::create(const char *name, OscObject *object1,
                                 OscObject *object2, double ax,
                                 double ay, double az)
{
    OscSlide *cons=NULL;
    cons = new OscSlideODE(simulation()->odeWorld(),
                           simulation()->odeSpace(),
                           name, m_parent, object1, object2,
                           ax, ay, az);

    if (cons)
        return simulation()->add_constraint(*cons);
}

bool PhysicsPistonFactory::create(const char *name, OscObject *object1,
                                  OscObject *object2, double x, double y,
                                  double z, double ax, double ay, double az)
{
    OscPiston *cons=NULL;
    cons = new OscPistonODE(simulation()->odeWorld(),
                            simulation()->odeSpace(),
                            name, m_parent, object1, object2,
                            x, y, z, ax, ay, az);

    if (cons)
        return simulation()->add_constraint(*cons);
}

bool PhysicsUniversalFactory::create(const char *name, OscObject *object1,
                                     OscObject *object2, double x,
                                     double y, double z, double a1x,
                                     double a1y, double a1z, double a2x,
                                     double a2y, double a2z)
{
    OscUniversal *cons=NULL;
    cons = new OscUniversalODE(simulation()->odeWorld(),
                               simulation()->odeSpace(),
                               name, m_parent, object1, object2,
                               x, y, z, a1x, a1y, a1z, a2x, a2y, a2z);

    if (cons)
        return simulation()->add_constraint(*cons);
}

/****** PhysicsSim ******/

const int PhysicsSim::MAX_CONTACTS = 30;

PhysicsSim::PhysicsSim(const char *port)
    : Simulation(port, ST_PHYSICS)
{
    m_pPrismFactory = new PhysicsPrismFactory(this);
    m_pSphereFactory = new PhysicsSphereFactory(this);
    m_pHingeFactory = new PhysicsHingeFactory(this);
    m_pHinge2Factory = new PhysicsHinge2Factory(this);
    m_pFixedFactory = new PhysicsFixedFactory(this);
    m_pBallJointFactory = new PhysicsBallJointFactory(this);
    m_pSlideFactory = new PhysicsSlideFactory(this);
    m_pPistonFactory = new PhysicsPistonFactory(this);
    m_pUniversalFactory = new PhysicsUniversalFactory(this);

    m_pGrabbedObject = NULL;

    m_fTimestep = PHYSICS_TIMESTEP_MS/1000.0;
    m_counter = 0;
    printf("ODE timestep: %f\n", m_fTimestep);
}

PhysicsSim::~PhysicsSim()
{
}

void PhysicsSim::initialize()
{
    dInitODE();

    dSetDebugHandler(ode_errorhandler);
    dSetErrorHandler(ode_errorhandler);
    dSetMessageHandler(ode_errorhandler);

    m_odeWorld = dWorldCreate();
    dWorldSetGravity (m_odeWorld,0,0,0);
    m_odeSpace = dSimpleSpaceCreate(0);
    m_odeContactGroup = dJointGroupCreate(0);

    /* This is just to track haptics cursor during "grab" state.
     * We only need its position, so just use a generic OscObject. */
    m_pCursor = new OscObject(NULL, "cursor", this);
    if (!m_pCursor)
        printf("Error creating PhysicsSim cursor.\n");

    Simulation::initialize();
}

void PhysicsSim::step()
{
    // Add extra forces to objects
    // Grabbed object attraction
    if (m_pGrabbedObject)
    {
        cVector3d grab_force(m_pGrabbedObject->getPosition()
                             - m_pCursor->m_position);

        grab_force.mul(-0.01);
        grab_force.add(m_pGrabbedObject->getVelocity()*(-0.0003));
        dBodyAddForce(m_pGrabbedObject->body(),
                      grab_force.x, grab_force.y, grab_force.z);
    }

    // Perform simulation step
	dSpaceCollide (m_odeSpace, this, &ode_nearCallback);
	dWorldStepFast1 (m_odeWorld, m_fTimestep, 5);
    /*
	for (int j = 0; j < dSpaceGetNumGeoms(ode_space); j++){
		dSpaceGetGeom(ode_space, j);
	}
    */
	dJointGroupEmpty (m_odeContactGroup);

    /* Update positions of each object in the other simulations */
    std::map<std::string,OscObject*>::iterator it;
    for (it=world_objects.begin(); it!=world_objects.end(); it++)
    {
        // TODO: it would be very nice to do this without involving dynamic_cast
        ODEObject *o = dynamic_cast<ODEObject*>(it->second);
        if (o) {
            o->update();
            cVector3d pos(o->getPosition());
            cMatrix3d rot(o->getRotation());
            send(true, (it->second->path()+"/position").c_str(), "fff",
                 pos.x,pos.y,pos.z);
            send(true, (it->second->path()+"/rotation").c_str(), "fffffffff",
                 rot.m[0][0], rot.m[0][1], rot.m[0][2],
                 rot.m[1][0], rot.m[1][1], rot.m[1][2],
                 rot.m[2][0], rot.m[2][1], rot.m[2][2]);
        }
    }

    /* Update the responses of each constraint. */
    std::map<std::string,OscConstraint*>::iterator cit;
    for (cit=world_constraints.begin(); cit!=world_constraints.end(); cit++)
    {
        cit->second->simulationCallback();
    }

    m_counter++;
}

void PhysicsSim::ode_nearCallback (void *data, dGeomID o1, dGeomID o2)
{
    PhysicsSim *me = static_cast<PhysicsSim*>(data);

    int i;
    // if (o1->body && o2->body) return;

    // exit without doing anything if the two bodies are connected by a joint
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);
    if (b1 && b2 && dAreConnectedExcluding (b1,b2,dJointTypeContact)) return;

    dContact contact[MAX_CONTACTS];   // up to MAX_CONTACTS contacts per box-box
    for (i=0; i<MAX_CONTACTS; i++) {
        contact[i].surface.mode = dContactBounce | dContactSoftCFM;
        contact[i].surface.mu = dInfinity;
        contact[i].surface.mu2 = 0;
        contact[i].surface.bounce = 0.1;
        contact[i].surface.bounce_vel = 0.1;
        contact[i].surface.soft_cfm = 0.01;
    }

	if (int numc = dCollide (o1,o2,MAX_CONTACTS,&contact[0].geom,sizeof(dContact)))
	{
        OscObject *p1 = static_cast<OscObject*>(dGeomGetData(o1));
        OscObject *p2 = static_cast<OscObject*>(dGeomGetData(o2));
        if (p1 && p2) {
            bool co1 = p1->collidedWith(p2, me->m_counter);
            bool co2 = p2->collidedWith(p1, me->m_counter);
            if ( (co1 || co2) && me->m_collide.m_value ) {
                lo_send(address_send, "/world/collide", "ssf",
                        p1->c_name(), p2->c_name(),
                        (double)(p1->m_velocity - p2->m_velocity).length());
            }
            // TODO: this strategy will NOT work for multiple collisions between same objects!!
        }
		for (i=0; i<numc; i++) {
			dJointID c = dJointCreateContact (me->m_odeWorld, me->m_odeContactGroup, contact+i);
			dJointAttach (c,b1,b2);
		}
	}
}

void PhysicsSim::set_grabbed(OscObject *pGrabbed)
{
    m_pGrabbedObject = dynamic_cast<ODEObject*>(pGrabbed);
}

/****** ODEObject ******/

ODEObject::ODEObject(dWorldID odeWorld, dSpaceID odeSpace)
    : m_odeWorld(odeWorld), m_odeSpace(odeSpace)
{
    m_odeGeom = NULL;
    m_odeBody = NULL;
    m_odeBody = dBodyCreate(m_odeWorld);
    dBodySetPosition(m_odeBody, 0, 0, 0);
}

ODEObject::~ODEObject()
{
    if (m_odeBody)  dBodyDestroy(m_odeBody);
    if (m_odeGeom)  dGeomDestroy(m_odeGeom);
}

void ODEObject::update()
{
    /* TODO: Again, would be nice to avoid dynamic_cast here! */
    OscObject *o = dynamic_cast<OscObject*>(this);
    if (!o) return;

    cVector3d pos(getPosition());
    cVector3d vel(o->m_position - pos);
    o->m_accel.set(o->m_velocity - vel);
    o->m_velocity.set(vel);
    o->m_position.set(pos);
}

/****** ODEConstraint ******/

ODEConstraint::ODEConstraint(dWorldID odeWorld, dSpaceID odeSpace,
                             OscObject *object1, OscObject *object2)
{
    m_odeWorld = odeWorld;
    m_odeSpace = odeSpace;
    m_odeBody1 = 0;
    m_odeBody2 = 0;
    m_odeJoint = 0;

    ODEObject *o = dynamic_cast<ODEObject*>(object1);
    if (o)
        m_odeBody1 = o->m_odeBody;

    if (!object2) {
        printf("constraint created with bodies %#x and world.\n", m_odeBody1);
        return;
    }

    o = dynamic_cast<ODEObject*>(object2);
    if (o)
        m_odeBody2 = o->m_odeBody;

    printf("constraint created with bodies %#x and %#x.\n", m_odeBody1, m_odeBody2);
}

ODEConstraint::~ODEConstraint()
{
    if (m_odeJoint)
        dJointDestroy(m_odeJoint);
}

/****** OscSphereODE ******/

OscSphereODE::OscSphereODE(dWorldID odeWorld, dSpaceID odeSpace, const char *name, OscBase *parent)
    : OscSphere(NULL, name, parent), ODEObject(odeWorld, odeSpace)
{
    m_odeGeom = dCreateSphere(m_odeSpace, 0.01);
    dGeomSetPosition(m_odeGeom, 0, 0, 0);
    dMassSetSphere(&m_odeMass, m_density.m_value, 0.01);
    dBodySetMass(m_odeBody, &m_odeMass);
    dGeomSetBody(m_odeGeom, m_odeBody);
    dBodySetPosition(m_odeBody, 0, 0, 0);
    dGeomSetData(m_odeGeom, static_cast<OscObject*>(this));

    addHandler("push", "ffffff", OscSphereODE::push_handler);
}

void OscSphereODE::on_radius()
{
    dGeomSphereSetRadius(m_odeGeom, m_radius.m_value);

    // reset the mass to maintain same density
    dMassSetSphere(&m_odeMass, m_density.m_value, m_radius.m_value);
    dBodySetMass(m_odeBody, &m_odeMass);

    m_mass.m_value = m_odeMass.mass;
}

void OscSphereODE::on_force()
{
    dBodyAddForce(m_odeBody, m_force.x, m_force.y, m_force.z);
}

void OscSphereODE::on_mass()
{
    dMassSetSphereTotal(&m_odeMass, m_mass.m_value, m_radius.m_value);
    dBodySetMass(m_odeBody, &m_odeMass);

    dReal volume = 4*M_PI*m_radius.m_value*m_radius.m_value*m_radius.m_value/3;
    m_density.m_value = m_mass.m_value / volume;
}

void OscSphereODE::on_density()
{
    dMassSetSphere(&m_odeMass, m_density.m_value, m_radius.m_value);
    dBodySetMass(m_odeBody, &m_odeMass);

    m_mass.m_value = m_odeMass.mass;
}

int OscSphereODE::push_handler(const char *path, const char *types,
                               lo_arg **argv, int argc,
                               void *data, void *user_data)
{
    OscSphereODE *me = static_cast<OscSphereODE*>(user_data);
    me->m_force.set(argv[0]->f, argv[1]->f, argv[2]->f);
    dBodyAddForceAtPos(me->m_odeBody,
                       argv[0]->f, argv[1]->f, argv[2]->f,
                       argv[3]->f, argv[4]->f, argv[5]->f);
    return 0;
}

/****** OscPrismODE ******/

OscPrismODE::OscPrismODE(dWorldID odeWorld, dSpaceID odeSpace, const char *name, OscBase *parent)
    : OscPrism(NULL, name, parent), ODEObject(odeWorld, odeSpace)
{
    m_odeGeom = dCreateBox(m_odeSpace, 0.01, 0.01, 0.01);
    dGeomSetPosition(m_odeGeom, 0, 0, 0);
    dMassSetBox(&m_odeMass, m_density.m_value, 0.01, 0.01, 0.01);
    dBodySetMass(m_odeBody, &m_odeMass);
    dGeomSetBody(m_odeGeom, m_odeBody);
    dBodySetPosition(m_odeBody, 0, 0, 0);
    dGeomSetData(m_odeGeom, static_cast<OscObject*>(this));

    addHandler("push", "ffffff", OscPrismODE::push_handler);
}

void OscPrismODE::on_size()
{
    if (m_size.x <= 0)
        m_size.x = 0.0001;
    if (m_size.y <= 0)
        m_size.y = 0.0001;
    if (m_size.z <= 0)
        m_size.z = 0.0001;

    // resize ODE geom
    dGeomBoxSetLengths (m_odeGeom, m_size[0], m_size[1], m_size[2]);

    // reset the mass to maintain same density
    dMassSetBox(&m_odeMass, m_density.m_value,
                m_size[0], m_size[1], m_size[2]);
    dBodySetMass(m_odeBody, &m_odeMass);

    m_mass.m_value = m_odeMass.mass;
}

void OscPrismODE::on_force()
{
    dBodyAddForce(m_odeBody, m_force.x, m_force.y, m_force.z);
}

void OscPrismODE::on_mass()
{
    dMassSetBoxTotal(&m_odeMass, m_mass.m_value,
                     m_size.x, m_size.y, m_size.z);
    dBodySetMass(m_odeBody, &m_odeMass);

    dReal volume = m_size.x * m_size.y * m_size.z;
    m_density.m_value = m_mass.m_value / volume;
}

void OscPrismODE::on_density()
{
    dMassSetBox(&m_odeMass, m_density.m_value,
                m_size.x, m_size.y, m_size.z);
    dBodySetMass(m_odeBody, &m_odeMass);

    m_mass.m_value = m_odeMass.mass;
}

int OscPrismODE::push_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data)
{
    OscPrismODE *me = static_cast<OscPrismODE*>(user_data);
    me->m_force.set(argv[0]->f, argv[1]->f, argv[2]->f);
    dBodyAddForceAtPos(me->m_odeBody,
                       argv[0]->f, argv[1]->f, argv[2]->f,
                       argv[3]->f, argv[4]->f, argv[5]->f);
    return 0;
}

//! A hinge requires a fixed anchor point and an axis
OscHingeODE::OscHingeODE(dWorldID odeWorld, dSpaceID odeSpace,
                         const char *name, OscBase* parent,
                         OscObject *object1, OscObject *object2,
                         double x, double y, double z, double ax, double ay, double az)
    : OscHinge(name, parent, object1, object2, x, y, z, ax, ay, az),
      ODEConstraint(odeWorld, odeSpace, object1, object2)
{
    m_response = new OscResponse("response",this);

	// create the constraint for object1
    cVector3d anchor(x,y,z);
    cVector3d axis(ax,ay,az);
    
    m_odeJoint = dJointCreateHinge(m_odeWorld,0);
    dJointAttach(m_odeJoint, m_odeBody1, m_odeBody2);
    dJointSetHingeAnchor(m_odeJoint, anchor.x, anchor.y, anchor.z);
    dJointSetHingeAxis(m_odeJoint, axis.x, axis.y, axis.z);

    printf("Hinge joint created between %s and %s at anchor (%f,%f,%f), axis (%f,%f,%f)\n",
        object1->c_name(), object2?object2->c_name():"world", x,y,z,ax,ay,az);
}

OscHingeODE::~OscHingeODE()
{
    delete m_response;
}

//! This function is called once per simulation step, allowing the
//! constraint to be "motorized" according to some response.  It runs
//! in the physics thread.
void OscHingeODE::simulationCallback()
{
    ODEConstraint& me = *static_cast<ODEConstraint*>(this);

    dReal angle = dJointGetHingeAngle(me.joint());
    dReal rate = dJointGetHingeAngleRate(me.joint());

    dReal addtorque =
        - m_response->m_stiffness.m_value*angle
        - m_response->m_damping.m_value*rate;

    // Limit the torque otherwise we get ODE assertions.
    if (addtorque >  1000) addtorque =  1000;
    if (addtorque < -1000) addtorque = -1000;

    m_torque.set(addtorque);

    dJointAddHingeTorque(me.joint(), addtorque);
}

OscHinge2ODE::OscHinge2ODE(dWorldID odeWorld, dSpaceID odeSpace,
                           const char *name, OscBase *parent,
                           OscObject *object1, OscObject *object2,
                           double x, double y, double z, double a1x,
                           double a1y, double a1z, double a2x,
                           double a2y, double a2z)
    : OscHinge2(name, parent, object1, object2, x, y, z,
                a1x, a1y, a1z, a2x, a2y, a2z),
      ODEConstraint(odeWorld, odeSpace, object1, object2)
{
    m_response = new OscResponse("response",this);

    m_odeJoint = dJointCreateHinge2(m_odeWorld,0);
    dJointAttach(m_odeJoint, m_odeBody1, m_odeBody2);
    dJointSetHinge2Anchor(m_odeJoint, x, y, z);
    dJointSetHinge2Axis1(m_odeJoint, a1x, a1y, a1z);
    dJointSetHinge2Axis2(m_odeJoint, a2x, a2y, a2z);

    printf("[%s] Hinge2 joint created between %s and %s at (%f, %f, %f) for axes (%f, %f, %f) and (%f,%f,%f)\n",
           simulation()->type_str(),
           object1->c_name(), object2?object2->c_name():"world",
           x, y, z, a1x, a1y, a1z, a2x, a2y, a2z);
}

OscHinge2ODE::~OscHinge2ODE()
{
    delete m_response;
}

//! This function is called once per simulation step, allowing the
//! constraint to be "motorized" according to some response.
void OscHinge2ODE::simulationCallback()
{
    ODEConstraint& me = *static_cast<ODEConstraint*>(this);

    dReal angle1 = dJointGetHinge2Angle1(me.joint());
    dReal rate1 = dJointGetHinge2Angle1Rate(me.joint());

    dReal addtorque1 =
        - m_response->m_stiffness.m_value*angle1
        - m_response->m_damping.m_value*rate1;

#if 0  // TODO: dJointGetHinge2Angle2 is not yet available in ODE.
    dReal angle2 = dJointGetHinge2Angle2(me.joint());
#else
    dReal angle2 = 0;
#endif
    dReal rate2 = dJointGetHinge2Angle2Rate(me.joint());

    dReal addtorque2 =
        - m_response->m_stiffness.m_value*angle2
        - m_response->m_damping.m_value*rate2;

    dJointAddHinge2Torques(me.joint(), addtorque1, addtorque2);
}

OscFixedODE::OscFixedODE(dWorldID odeWorld, dSpaceID odeSpace,
                         const char *name, OscBase* parent,
                         OscObject *object1, OscObject *object2)
    : OscFixed(name, parent, object1, object2),
      ODEConstraint(odeWorld, odeSpace, object1, object2)
{
    if (m_odeBody2) {
        m_odeJoint = dJointCreateFixed(m_odeWorld,0);
        dJointAttach(m_odeJoint, m_odeBody1, m_odeBody2);
        dJointSetFixed(m_odeJoint);
    }
    else {
        ODEObject *o = dynamic_cast<ODEObject*>(object1);
        if (o)
            o->disconnectBody();
    }

    printf("[%s] Fixed joint created between %s and %s.\n",
           simulation()->type_str(),
        object1->c_name(), object2?object2->c_name():"world");
}


OscBallJointODE::OscBallJointODE(dWorldID odeWorld, dSpaceID odeSpace,
                                 const char *name, OscBase *parent,
                                 OscObject *object1, OscObject *object2,
                                 double x, double y, double z)
    : OscBallJoint(name, parent, object1, object2, x, y, z),
      ODEConstraint(odeWorld, odeSpace, object1, object2)
{
    m_odeJoint = dJointCreateBall(m_odeWorld,0);
    dJointAttach(m_odeJoint, m_odeBody1, m_odeBody2);
    dJointSetBallAnchor(m_odeJoint, x, y, z);

    printf("[%s] Ball joint created between %s and %s at (%f,%f,%f)\n",
           simulation()->type_str(),
           object1->c_name(), object2?object2->c_name():"world",
           x, y, z);
}

OscSlideODE::OscSlideODE(dWorldID odeWorld, dSpaceID odeSpace,
                         const char *name, OscBase *parent,
                         OscObject *object1, OscObject *object2,
                         double ax, double ay, double az)
    : OscSlide(name, parent, object1, object2, ax, ay, az),
      ODEConstraint(odeWorld, odeSpace, object1, object2)
{
    m_odeJoint = dJointCreateSlider(m_odeWorld,0);
    dJointAttach(m_odeJoint, m_odeBody1, m_odeBody2);
    dJointSetSliderAxis(m_odeJoint, ax, ay, az);
    /* TODO access to dJointGetSliderPosition */

    printf("[%s] Sliding joint created between %s and %s on axis (%f,%f,%f)\n",
           simulation()->type_str(),
           object1->c_name(), object2?object2->c_name():"world",
           ax, ay, az);
}

//! A piston requires a fixed anchor point and an axis
OscPistonODE::OscPistonODE(dWorldID odeWorld, dSpaceID odeSpace,
                           const char *name, OscBase* parent,
                           OscObject *object1, OscObject *object2,
                           double x, double y, double z,
                           double ax, double ay, double az)
    : OscPiston(name, parent, object1, object2, x, y, z, ax, ay, az),
      ODEConstraint(odeWorld, odeSpace, object1, object2)
{
	// create the constraint for object1
    cVector3d anchor(x,y,z);
    cVector3d axis(ax,ay,az);

    m_odeJoint = dJointCreatePiston(m_odeWorld,0);
    dJointAttach(m_odeJoint, m_odeBody1, m_odeBody2);
    dJointSetPistonAnchor(m_odeJoint, anchor.x, anchor.y, anchor.z);
    dJointSetPistonAxis(m_odeJoint, axis.x, axis.y, axis.z);

    printf("Piston joint created between %s and %s at anchor (%f,%f,%f), axis (%f,%f,%f)\n",
        object1->c_name(), object2?object2->c_name():"world", x,y,z,ax,ay,az);
}

OscUniversalODE::OscUniversalODE(dWorldID odeWorld, dSpaceID odeSpace,
                                 const char *name, OscBase *parent,
                                 OscObject *object1, OscObject *object2,
                                 double x, double y, double z, double a1x,
                                 double a1y, double a1z, double a2x,
                                 double a2y, double a2z)
    : OscUniversal(name, parent, object1, object2, x, y, z,
                   a1x, a1y, a1z, a2x, a2y, a2z),
      ODEConstraint(odeWorld, odeSpace, object1, object2)
{
    m_response = new OscResponse("response",this);

    m_odeJoint = dJointCreateUniversal(m_odeWorld,0);
    dJointAttach(m_odeJoint, m_odeBody1, m_odeBody2);
    dJointSetUniversalAnchor(m_odeJoint, x, y, z);
    dJointSetUniversalAxis1(m_odeJoint, a1x, a1y, a1z);
    dJointSetUniversalAxis2(m_odeJoint, a2x, a2y, a2z);

    printf("[%s] Universal joint created between %s and %s at (%f, %f, %f) for axes (%f, %f, %f) and (%f,%f,%f)\n",
           simulation()->type_str(),
           object1->c_name(), object2?object2->c_name():"world",
           x, y, z, a1x, a1y, a1z, a2x, a2y, a2z);
}

OscUniversalODE::~OscUniversalODE()
{
    delete m_response;
}

//! This function is called once per simulation step, allowing the
//! constraint to be "motorized" according to some response.
void OscUniversalODE::simulationCallback()
{
    ODEConstraint& me = *static_cast<ODEConstraint*>(this);

    dReal angle1 = dJointGetUniversalAngle1(me.joint());
    dReal rate1 = dJointGetUniversalAngle1Rate(me.joint());

    dReal addtorque1 =
        - m_response->m_stiffness.m_value*angle1
        - m_response->m_damping.m_value*rate1;

    dReal angle2 = dJointGetUniversalAngle2(me.joint());
    dReal rate2 = dJointGetUniversalAngle2Rate(me.joint());

    dReal addtorque2 =
        - m_response->m_stiffness.m_value*angle2
        - m_response->m_damping.m_value*rate2;

    dJointAddUniversalTorques(me.joint(), addtorque1, addtorque2);
}
