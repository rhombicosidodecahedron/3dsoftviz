#ifndef ARCONTROLOBJECT_H
#define ARCONTROLOBJECT_H

#include <osg/Vec3f>
#include "Data/Node.h"
#include "Core/Core.h"

namespace Data {
class Graph;
}
namespace ArucoModul {

//concrete objects
class ArControlObject : public QObject
{

public:
    ArControlObject(int id, osg::Vec3f position , qlonglong nodeToPick);
    void updatePosition( osg::Vec3f position );
    bool isLost(){
        return this->lost;
    }

    void setObjectAsLost();

private:
    int id;
    qlonglong nodeToPick;
    osg::Vec3f position;
    bool focused;
    bool lost;

    osg::ref_ptr<Data::Node> focusedNode;

    bool chckIfNearPosition( osg::Vec3f target );
};


//object controller
class ArControlClass : public QObject
{

public:
    ArControlClass();
    void updateObjectPositionAruco(qlonglong object_id, QMatrix4x4 modelViewMatrix , bool reverse);

private:
    QOSG::ViewerQT* viewer;
    Vwr::CoreGraph* coreGraph;

    QMap<qlonglong, ArucoModul::ArControlObject*> controlObjects;
};

} // end ArucoModul namespace

#endif // ARCONTROLOBJECT_H
