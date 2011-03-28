#include "Layout/FRAlgorithm.h"

using namespace Layout;
using namespace Vwr;	



 //Konstruktor pre vlakno s algoritmom 
FRAlgorithm::FRAlgorithm() 
{
	PI = acos((double) - 1);
	ALPHA = 0.005;
	MIN_MOVEMENT = 0.05;
	MAX_MOVEMENT = 30;
	MAX_DISTANCE = 400;	
	state = RUNNING;
	notEnd = true;	
	center = osg::Vec3f (0,0,0);
	fv = osg::Vec3f();
	last = osg::Vec3f();
	newLoc = osg::Vec3f();
	up = osg::Vec3f();
	vp = osg::Vec3f();	
	
	/* moznost odpudiveho posobenia limitovaneho vzdialenostou*/
	useMaxDistance = false;
	isIterating = false;
	this->graph = NULL;
}
FRAlgorithm::FRAlgorithm(Data::Graph *graph) 
{
	PI = acos((double) - 1);
	ALPHA = 0.005;
	MIN_MOVEMENT = 0.05;
	MAX_MOVEMENT = 30;
	MAX_DISTANCE = 400;	
	state = RUNNING;
	notEnd = true;
	osg::Vec3f p(0,0,0);	
	center = p;	
	fv = osg::Vec3f();
	last = osg::Vec3f();
	newLoc = osg::Vec3f();
	up = osg::Vec3f();
	vp = osg::Vec3f();	
	
	/* moznost odpudiveho posobenia limitovaneho vzdialenostou*/
	useMaxDistance = false;
	isIterating = false;
	this->graph = graph;
	this->Randomize();
}

void FRAlgorithm::SetGraph(Data::Graph *graph)
{	
	notEnd = true;
	this->graph = graph;
	this->Randomize();
}
void FRAlgorithm::SetParameters(float sizeFactor,float flexibility,int animationSpeed,bool useMaxDistance) 
{
	this->sizeFactor = sizeFactor;
	this->flexibility = flexibility;
	this->useMaxDistance = useMaxDistance;

	if(this->graph != NULL)
	{
		K = computeCalm();
		graph->setFrozen(false);
	}
	else
	{
		cout << "Nenastaveny graf. Pouzi metodu SetGraph(Data::Graph graph).";
	}
}

/* Urci pokojovu dlzku strun */
double FRAlgorithm::computeCalm() {
	double R = 300;
	float n = graph->getNodes()->count();
	return sizeFactor* pow((4*R*R*R*PI)/(n*3), 1/3);
}
/* Rozmiestni uzly na nahodne pozicie */
void FRAlgorithm::Randomize() 
{
    QMap<qlonglong, osg::ref_ptr<Data::Node> >::iterator j;
	j = graph->getNodes()->begin();

	for (int i = 0; i < graph->getNodes()->count(); i++,++j)
	{		
		if(!j.value()->isFixed())
		{
			osg::Vec3f randPos = getRandomLocation();
			j.value()->setTargetPosition(randPos);
		}
	}	
	graph->setFrozen(false);
}

osg::Vec3f FRAlgorithm::getRandomLocation() 
{
	double l = getRandomDouble() * 300;	
	double alpha = getRandomDouble() * 2 * PI;
	double beta = getRandomDouble() * 2 * PI;
	osg::Vec3f newPos =  osg::Vec3f((float) (l * sin(alpha)), (float) (l * cos(alpha) * cos(beta)), (float) (l	* cos(alpha) * sin(beta)));
	return newPos;
}
double FRAlgorithm::getRandomDouble()
{
	int p = rand();

	return (double)rand() / (double)RAND_MAX;
}

void FRAlgorithm::PauseAlg() 
{	
	state = PAUSED;
}

void FRAlgorithm::WakeUpAlg() 
{
	if(graph != NULL && state == RUNNING && graph->isFrozen())
	{
		graph->setFrozen(false);
	}
}

void FRAlgorithm::RunAlg() 
{
	if(graph != NULL)
	{
		K = computeCalm();
		graph->setFrozen(false);
		state = RUNNING;
		notEnd = true;
	}
}

bool FRAlgorithm::IsRunning() 
{
	return isIterating;
}

void FRAlgorithm::terminate() 
{
	notEnd = false;
}

void FRAlgorithm::Run() 
{
	if(this->graph != NULL)
	{
		isIterating = true;
		while (notEnd) 
		{			
			// slucka pozastavenia - ak je pauza
			// alebo je graf zmrazeny (spravidla pocas editacie)
			while (state != RUNNING || graph->isFrozen()) 
			{				
				QThread::msleep(100);				
				if(state == PAUSED)
				{
					if(isIterating)
					{
						isIterating = false;
					}					
				}							
			}
			if(!isIterating)
			{
				isIterating = true;
			}
			if (!iterate()) {
				graph->setFrozen(true);	
			}			
		}
	}
	else
	{
		cout << "Nenastaveny graf. Pouzi metodu SetGraph(Data::Graph graph).";

	}
}

bool FRAlgorithm::iterate()
{	
	bool changed = false;  		
	{			
        QMap<qlonglong, osg::ref_ptr<Data::Node> >::iterator j;
		j = graph->getNodes()->begin();
		for (int i = 0; i < graph->getNodes()->count(); i++,++j)
		{ // pre vsetky uzly..
			Data::Node* node = j.value();
			node->resetForce(); // vynulovanie posobiacej sily			
		}		
	}
	{//meta uzly
		
		QMap<qlonglong, osg::ref_ptr<Data::Node> >::iterator j;
		QMap<qlonglong, osg::ref_ptr<Data::Node> >::iterator k;	
		j = graph->getMetaNodes()->begin();
		for (int i = 0; i < graph->getMetaNodes()->count(); i++,++j)
		{ // pre vsetky metauzly..
			j.value()->resetForce(); // vynulovanie posobiacej sily
			k = graph->getMetaNodes()->begin();
			for (int h = 0; h < graph->getMetaNodes()->count(); h++,++k)
			{ // pre vsetky metauzly..
				if (!j.value()->equals(k.value())) 
				{
					// odpudiva sila medzi metauzlami
					addRepulsive(j.value(), k.value(), Data::Graph::getMetaStrength());
				}
			}
		}		
	}
	{//meta hrany
		
		QMap<qlonglong, osg::ref_ptr<Data::Edge> >::iterator j;
		j = graph->getMetaEdges()->begin();
		for (int i = 0; i < graph->getMetaEdges()->count(); i++,++j)
		{ // pre vsetky metahrany..
			Data::Node *u = j.value()->getSrcNode();
			Data::Node *v = j.value()->getDstNode();
			// uzly nikdy nebudu ignorovane
			/*
			u->setIgnored(false);
			v->setIgnored(false);
			*/ // ignored value has not been used, so setting it here did not have any effect
			if (graph->getMetaNodes()->contains(u->getId())) {
				// pritazliva sila, posobi na v
				addMetaAttractive(v, u, Data::Graph::getMetaStrength());
			}
			if (graph->getMetaNodes()->contains(v->getId())) {
				// pritazliva sila, posobi na u
				addMetaAttractive(u, v, Data::Graph::getMetaStrength());
			}
		}
	}
	{//uzly
        QMap<qlonglong, osg::ref_ptr<Data::Node> >::iterator j;
        QMap<qlonglong, osg::ref_ptr<Data::Node> >::iterator k;
		j = graph->getNodes()->begin();
		for (int i = 0; i < graph->getNodes()->count(); i++,++j) 
		{ // pre vsetky uzly..
			k = graph->getNodes()->begin();
			for (int h = 0; h < graph->getNodes()->count(); h++,++k) { // pre vsetky uzly..
				if (!j.value()->equals(k.value())) {
					// odpudiva sila beznej velkosti
					addRepulsive(j.value(), k.value(), 1);
				}
			}
		}
	}
	{//hrany
        QMap<qlonglong, osg::ref_ptr<Data::Edge> >::iterator j;
		j = graph->getEdges()->begin();
		for (int i = 0; i < graph->getEdges()->count(); i++,++j)
		{ // pre vsetky hrany..
			// pritazliva sila beznej velkosti
			addAttractive(j.value(), 1);
		}
	}	
	if(state == PAUSED) 
	{
		return true;
	}
	
	// aplikuj sily na uzly
	{	
        QMap<qlonglong, osg::ref_ptr<Data::Node> >::iterator j;
		j = graph->getNodes()->begin();
		for (int i = 0; i < graph->getNodes()->count(); i++,++j)
		{ // pre vsetky uzly..
			if (!j.value()->isFixed()) {
				last = j.value()->getTargetPosition();
				bool fo = applyForces(j.value());
				changed = changed || fo;
			}
			else
			{
				changed = false;
			}
		}
	}
	// aplikuj sily na metauzly
	{
		QMap<qlonglong, osg::ref_ptr<Data::Node> >::iterator j;
		j = graph->getMetaNodes()->begin();
		for (int i = 0; i < graph->getMetaNodes()->count(); i++,++j) 
		{ // pre vsetky metauzly..
			if (!j.value()->isFixed()) {
				bool fo = applyForces(j.value());
				changed = changed || fo;
			}
		}
	}
	// vracia true ak sa ma pokracovat dalsou iteraciou

	return changed;
}

bool FRAlgorithm::applyForces(Data::Node* node) 
{
	// nakumulovana sila
	osg::Vec3f fv = node->getForce();
	// zmensenie
	fv *= ALPHA;
	float l = fv.length();
	if (l > MIN_MOVEMENT)
	{ // nie je sila primala?
		if (l > MAX_MOVEMENT)
		{ // je sila privelka?			
			fv.normalize();
			fv *= 5;
		}

		// pricitame aktualnu rychlost
		fv += node->getVelocity();

		osg::Vec3f computedTargetPosition = node->getTargetPosition () + fv;
		osg::Vec3f restrictedTargetPosition = graph->getRestrictionsManager ().applyRestriction (*node, computedTargetPosition);

		node->setTargetPosition(restrictedTargetPosition);

		// energeticka strata = 1-flexibilita
		fv *= flexibility;
		node->setVelocity(fv); // ulozime novu rychlost

		return true;
	} else {
		node->resetVelocity(); // vynulovanie rychlosti
		return false;
	}
}

/* Pricitanie pritazlivych sil */
void FRAlgorithm::addAttractive(Data::Edge* edge, float factor) {
	if (!areForcesBetween (edge->getSrcNode(), edge->getDstNode())) {
		return;
	}
	up = edge->getSrcNode()->getTargetPosition();
	vp = edge->getDstNode()->getTargetPosition();
	dist = distance(up,vp);
	if (dist == 0)
		return;
	fv = vp - up; // smer sily
	fv.normalize();
	fv *= attr(dist) * factor;// velkost sily
	edge->getSrcNode()->addForce(fv);
	fv = center - fv;
	edge->getDstNode()->addForce(fv);
}

/* Pricitanie pritazlivych sil od metazla */
void FRAlgorithm::addMetaAttractive(Data::Node* u, Data::Node* meta, float factor) {
	if (!areForcesBetween (u, meta)) {
		return;
	}
	up = u->getTargetPosition();
	vp = meta->getTargetPosition();
	dist = distance(up,vp);
	if (dist == 0)
		return;
	fv = vp - up;// smer sily
	fv.normalize();
	fv *= attr(dist) * factor;// velkost sily
	u->addForce(fv);
}

/* Pricitanie odpudivych sil */
void FRAlgorithm::addRepulsive(Data::Node* u, Data::Node* v, float factor) {
	if (!areForcesBetween (u, v)) {
		return;
	}
	up = u->getTargetPosition();
	vp = v->getTargetPosition();
	dist = distance(up,vp);
	if (useMaxDistance && dist > MAX_DISTANCE) {
		return;
	}
	if (dist == 0) {
		// pri splynuti uzlov medzi nimi vytvorime malu vzdialenost
		vp.set(vp.x() + (rand() % 10), vp.y() + (rand() % 10), vp.z() + (rand() % 10));
		dist = distance(up,vp);
	}
	fv = (vp - up);// smer sily
	fv.normalize();
	fv *= rep(dist) * factor;// velkost sily
	u->addForce(fv);
}
/* Vzorec na vypocet odpudivej sily */
float FRAlgorithm::rep(double distance) {
	return (float) (-(K * K) / distance);
}

/* Vzorec na vypocet pritazlivej sily */
float FRAlgorithm::attr(double distance) {
	return (float) ((distance * distance) / K);
}

/* Vzorec na vypocet dostredivej sily */
float FRAlgorithm::centr(double distance) {
	return (float) distance;
}

double FRAlgorithm::distance(osg::Vec3f u,osg::Vec3f v)
{
	osg::Vec3f x = u - v;
	return (double) x.length();
}

bool FRAlgorithm::areForcesBetween (Data::Node * u, Data::Node * v) {
	return
		!(u->isIgnored ())
		&&
		!(v->isIgnored ())
		&&
		(
			graph->isInSameGraph (u, v)
			||
			u->getType ()->isMeta ()
			||
			v->getType ()->isMeta ()
		)
	;
}
