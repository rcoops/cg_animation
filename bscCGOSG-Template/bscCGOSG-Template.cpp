// bscCGOSG-Template.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <string>
#include <conio.h>
#include <windows.h>
#include <gl/gl.h>
#include <mmsystem.h>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgUtil/Optimizer>
#include <osgViewer/Viewer>
#include <osg/MatrixTransform>
#include <osgGA/TrackballManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgGA/StateSetManipulator>
#include <osgGA/GUIEventHandler>
#include <osg/Drawable>
#include <osg/ShapeDrawable>
#include <osgUtil/UpdateVisitor>
#include <osgViewer/ViewerEventHandlers>
#include <osg/Material>

osg::Node *g_pModel = 0;
osg::Group *g_pRoot = 0;

// uncomment only 1 of these
//#define example1
#define example2
//#define example3
//#define example4


// basic node printer
class nodePrinter : public osg::NodeVisitor
{
public:
	nodePrinter() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {};

	virtual void apply(osg::Node &node)
	{
		std::string s;
		for (unsigned int i = 0; i < getNodePath().size(); i++) s += "|--";

		std::cout << s << "Node -> " << node.getName() << " :: " << node.libraryName() << std::endl;

		traverse(node);
	}
};

// basic matrix transform finder
class mtFinder : public osg::NodeVisitor
{
public:
	mtFinder(std::string sName) : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), m_sName(sName), m_pMT(0) {};
	virtual ~mtFinder()
	{
		if (m_pMT) m_pMT->unref();
	}

	osg::MatrixTransform *mt()
	{
		return m_pMT;
	}

	virtual void apply(osg::Node &node)
	{
		if (dynamic_cast<osg::MatrixTransform*>(&node) && node.getName() == m_sName)
		{
			m_pMT = dynamic_cast<osg::MatrixTransform*>(&node);
			m_pMT->ref();
		}
		else
			traverse(node);
	}

protected:
	std::string m_sName;
	osg::MatrixTransform *m_pMT;
};

// generic finder
// basic matrix transform finder
template<class T>
class nodeFinder : public osg::NodeVisitor
{
public:
	nodeFinder(std::string sName) : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), m_sName(sName), m_pNode(0) {};
	virtual ~nodeFinder()
	{
		if (m_pNode) m_pNode->unref();
	}

	T *node()
	{
		return m_pNode;
	}

	virtual void apply(osg::Node &node)
	{
		if (dynamic_cast<T*>(&node) && node.getName() == m_sName)
		{
			m_pNode = dynamic_cast<T*>(&node);
			m_pNode->ref();
		}
		else
			traverse(node);
	}

protected:
	std::string m_sName;
	T *m_pNode;
};

#ifndef example1 

// basic ball animated object
class ballF : public osg::AnimationPathCallback
{
public:
	ballF(osg::AnimationPath *pPath)
	{
		m_pRoot = new osg::MatrixTransform();
		osg::Geode *pGeode = new osg::Geode();
		osg::ShapeDrawable *pSD = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3f(0.0f, 0.0f, 11.0f), 10.0f));
		pGeode->addDrawable(pSD);
		m_pRoot->addChild(pGeode);

		this->setAnimationPath(pPath);
		// this allows the facade to provide itself as an animation callback to the geometry
		m_pRoot->setUpdateCallback(this);
		m_pRoot->ref();
	}

	virtual ~ballF()
	{
		if (m_pRoot)
		{
			m_pRoot->removeUpdateCallback(this);
			m_pRoot->unref();
		}
	}

	osg::Node * root()
	{
		return m_pRoot;
	}

protected:
	osg::MatrixTransform *m_pRoot;
};

ballF *g_pBall = 0;
typedef std::list<ballF*>ballFs;
ballFs g_Balls;
#endif

// tile facarde class
#ifdef example1 
// example 1: basix tile
class tileF
{
public:
	tileF(osg::Node *pNode, float fX, float fY, osg::Vec3f vColour)
	{
		// contains its own little scene graph
		m_pRoot = new osg::MatrixTransform();
		osg::Matrixf m;
		m.makeTranslate(fX, fY, 0.0f);
		m_pRoot->setMatrix(m);
		m_pRoot->ref();
		// pNode is the definition of the geometry (this prevents having to recreate it for every tile)
		m_pRoot->addChild(pNode);

		osg::Material *pMat = new osg::Material();
		osg::Vec4f vAmb, vDiff, vSpe;

		vAmb.set(vColour[0] * 0.2f, vColour[1] * 0.2f, vColour[2] * 0.2f, 1.0f);
		vDiff.set(vColour[0] * 0.8f, vColour[1] * 0.8f, vColour[2] * 0.8f, 1.0f);
		vSpe.set(vColour[0] * 0.9f, vColour[1] * 0.9f, vColour[2] * 0.9f, 1.0f);

		pMat->setAmbient(osg::Material::FRONT_AND_BACK, vAmb);
		pMat->setDiffuse(osg::Material::FRONT_AND_BACK, vDiff);
		pMat->setSpecular(osg::Material::FRONT_AND_BACK, vSpe);

		m_pRoot->getOrCreateStateSet()->setAttributeAndModes(pMat, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
	}


	virtual ~tileF()
	{
		if (m_pRoot) m_pRoot->unref();
	}

	osg::Node* root()
	{
		return m_pRoot;
	}

protected:
	osg::MatrixTransform *m_pRoot;
};
#endif

#ifdef example2 
//example 2 - tile +animation
class tileF
{
public:
	tileF(osg::Node *pNode, float fX, float fY, osg::Vec3f vColour)
	{
		m_pRoot = new osg::MatrixTransform();
		osg::Matrixf m;
		m.makeTranslate(fX, fY, 0.0f);
		m_pRoot->setMatrix(m);
		m_pRoot->ref();
		m_pRoot->addChild(pNode);

		osg::Material *pMat = new osg::Material();
		osg::Vec4f vAmb, vDiff, vSpe;

		vAmb.set(vColour[0] * 0.2f, vColour[1] * 0.2f, vColour[2] * 0.2f, 1.0f);
		vDiff.set(vColour[0] * 0.8f, vColour[1] * 0.8f, vColour[2] * 0.8f, 1.0f);
		vSpe.set(vColour[0] * 0.9f, vColour[1] * 0.9f, vColour[2] * 0.9f, 1.0f);

		pMat->setAmbient(osg::Material::FRONT_AND_BACK, vAmb);
		pMat->setDiffuse(osg::Material::FRONT_AND_BACK, vDiff);
		pMat->setSpecular(osg::Material::FRONT_AND_BACK, vSpe);

		m_pRoot->getOrCreateStateSet()->setAttributeAndModes(pMat, osg::StateAttribute::ON);

		m_avAnimationPoints[0].set(30.0f, -30.0f, 0.0f);
		m_avAnimationPoints[1].set(30.0f, 30.0f, 0.0f);
		m_avAnimationPoints[2].set(-30.0f, 30.0f, 0.0f);
		m_avAnimationPoints[3].set(-30.0f, -30.0f, 0.0f);

		buildAnimationPointsVisual();
	}

	void buildAnimationPointsVisual()
	{
		osg::Geode* pGeode = new osg::Geode();

		osg::Material *pMat = new osg::Material();
		osg::Vec4f vAmb, vDiff, vSpe;

		vAmb.set(0.1f, 0.1f, 0.1f, 1.0f);
		vDiff.set(0.7f, 0.7f, 0.7f, 1.0f);
		vSpe.set(1.0f, 1.0f, 1.0f, 1.0f);

		pMat->setAmbient(osg::Material::FRONT_AND_BACK, vAmb);
		pMat->setDiffuse(osg::Material::FRONT_AND_BACK, vDiff);
		pMat->setSpecular(osg::Material::FRONT_AND_BACK, vSpe);

		pGeode->getOrCreateStateSet()->setAttributeAndModes(pMat, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);

		for(unsigned int i=0;i<4;i++) pGeode->addDrawable(new osg::ShapeDrawable(new osg::Box(m_avAnimationPoints[i], 5.0f)));
		m_pRoot->addChild(pGeode);
	}

	osg::Vec3f animationPoint(unsigned int iIndex, osg::Node *pRoot)
	{
		// this is just one transform - we need to find the cumulative effect of multiple -.-
		// however - computeLocalToWorld does this (calculates all node paths between m_pRoot (our node) and scene graph root pNode)
		return m_avAnimationPoints[iIndex] * osg::computeLocalToWorld(m_pRoot->getParentalNodePaths(pRoot)[0]);
	}

	virtual ~tileF()
	{
		if (m_pRoot) m_pRoot->unref();
	}

	osg::Node* root()
	{
		return m_pRoot;
	}

	

protected:
	osg::MatrixTransform *m_pRoot;
	osg::Vec3f m_avAnimationPoints[4];
};
#endif

#ifdef example3
// example 3 tile +animation + monitoring
class tileF : osg::Callback
{
public:

	tileF(osg::Node *pNode, float fX, float fY, osg::Vec3f vColour)
	{
		m_pRoot = new osg::MatrixTransform();
		osg::Matrixf m;
		m.makeTranslate(fX, fY, 0.0f);
		m_pRoot->setMatrix(m);
		m_pRoot->ref();
		m_pRoot->setUpdateCallback(this);

		osg::Group *pGroup = new osg::Group();
		pGroup->addChild(pNode);
		m_pRoot->addChild(pGroup);
		pGroup->getOrCreateStateSet()->setAttributeAndModes(makeMaterial(vColour), osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);

		m_pOnMaterial = makeMaterial(osg::Vec3f(0.0f, 1.0f, 0.0f));
		m_pOnMaterial->ref();

		m_pOffMaterial = makeMaterial(osg::Vec3f(1.0f, 0.0f, 0.0f));
		m_pOffMaterial->ref();


		m_avAnimationPoints[0].set(30.0f, -30.0f, 0.0f);
		m_avAnimationPoints[1].set(30.0f, 30.0f, 0.0f);
		m_avAnimationPoints[2].set(-30.0f, 30.0f, 0.0f);
		m_avAnimationPoints[3].set(-30.0f, -30.0f, 0.0f);
	
		m_pBoxMT = new osg::MatrixTransform();
		osg::Geode *pBoxGeode = new osg::Geode();
		osg::ShapeDrawable *pBoxSD = new osg::ShapeDrawable(new osg::Box(osg::Vec3f(-30.0f, -30.0f, 10.0f), 20.0f));

		m_pBoxMT->addChild(pBoxGeode);
		pBoxGeode->addDrawable(pBoxSD);
		pBoxGeode->getOrCreateStateSet()->setAttributeAndModes(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
		m_pBoxMT->getOrCreateStateSet()->setAttributeAndModes(m_pOffMaterial, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

		m_pRoot->addChild(m_pBoxMT);


		
	}

	osg::Material* makeMaterial(osg::Vec3f vColour)
	{
		osg::Material *pMat = new osg::Material();
		osg::Vec4f vAmb, vDiff, vSpe;

		vAmb.set(vColour[0] * 0.2f, vColour[1] * 0.2f, vColour[2] * 0.2f, 1.0f);
		vDiff.set(vColour[0] * 0.8f, vColour[1] * 0.8f, vColour[2] * 0.8f, 1.0f);
		vSpe.set(vColour[0] * 0.9f, vColour[1] * 0.9f, vColour[2] * 0.9f, 1.0f);

		pMat->setAmbient(osg::Material::FRONT_AND_BACK, vAmb);
		pMat->setDiffuse(osg::Material::FRONT_AND_BACK, vDiff);
		pMat->setSpecular(osg::Material::FRONT_AND_BACK, vSpe);

		return pMat;
	}

	osg::Vec3f animationPoint(unsigned int iIndex, osg::Node *pRoot)
	{
		osg::Matrixf m = osg::computeLocalToWorld(m_pRoot->getParentalNodePaths(pRoot)[0]);

		return m_avAnimationPoints[iIndex] * m;
	}

	virtual ~tileF()
	{
		if (m_pRoot) m_pRoot->unref();
	}

	osg::Node* root()
	{
		return m_pRoot;
	}

	virtual bool run(osg::Object* object, osg::Object* data)
	{
		if(m_pRoot && g_Balls.size())
		{
			osg::Matrixf mWorldToZone = osg::computeWorldToLocal(m_pBoxMT->getParentalNodePaths(m_pRoot)[0]);
			osg::BoundingSphere b = m_pBoxMT->getBound();

			unsigned int uiCount = 0;
			for(ballFs::iterator it=g_Balls.begin();it!=g_Balls.end();it++)
			{
				osg::Vec3f vPos(0.0f, 0.0f, 0.0f);

				osg::Matrixf mBallToWorld = osg::computeLocalToWorld((*it)->root()->getParentalNodePaths(m_pRoot)[0]);
				vPos = vPos*mBallToWorld;
				vPos = vPos*mWorldToZone;

				if (b.contains(vPos))
				{
					m_pBoxMT->getOrCreateStateSet()->setAttributeAndModes(m_pOnMaterial, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
					uiCount++;
				}
			}
			if(!uiCount) m_pBoxMT->getOrCreateStateSet()->setAttributeAndModes(m_pOffMaterial, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);


		}
		return true;
	}

protected:
	osg::MatrixTransform *m_pRoot;
	osg::MatrixTransform *m_pBoxMT;
	osg::Vec3f m_avAnimationPoints[4];

	osg::Material *m_pOffMaterial;
	osg::Material *m_pOnMaterial;
};
#endif


#ifdef example4
// example 3 tile +animation + monitoring + showing off
class tileF : osg::Callback
{
public:

	tileF(osg::Node *pNode, float fX, float fY, osg::Vec3f vColour)
	{
		m_bShow = true;
		m_pRoot = new osg::MatrixTransform();
		osg::Matrixf m;
		m.makeTranslate(fX, fY, 0.0f);
		m_pRoot->setMatrix(m);
		m_pRoot->ref();
		m_pRoot->setUpdateCallback(this);

		osg::Group *pGroup = new osg::Group();
		pGroup->addChild(pNode);
		m_pRoot->addChild(pGroup);
		pGroup->getOrCreateStateSet()->setAttributeAndModes(makeMaterial(vColour), osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);

		m_pOnMaterial = makeMaterial(osg::Vec3f(0.0f, 1.0f, 0.0f));
		m_pOnMaterial->ref();

		m_pOffMaterial = makeMaterial(osg::Vec3f(1.0f, 0.0f, 0.0f));
		m_pOffMaterial->ref();

		m_avAnimationPoints[0].set(30.0f, -30.0f, 0.0f);
		m_avAnimationPoints[1].set(30.0f, 30.0f, 0.0f);
		m_avAnimationPoints[2].set(-30.0f, 30.0f, 0.0f);
		m_avAnimationPoints[3].set(-30.0f, -30.0f, 0.0f);

		m_pBoxMT = new osg::MatrixTransform();
		osg::Geode *pBoxGeode = new osg::Geode();
		osg::ShapeDrawable *pBoxSD = new osg::ShapeDrawable(new osg::Box(osg::Vec3f(-30.0f, -30.0f, 10.0f), 20.0f));

		m_pSwitch = new osg::Switch();
		m_pBoxMT->addChild(pBoxGeode);
		pBoxGeode->addDrawable(pBoxSD);
		pBoxGeode->getOrCreateStateSet()->setAttributeAndModes(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
		m_pBoxMT->getOrCreateStateSet()->setAttributeAndModes(m_pOffMaterial, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

		m_pRoot->addChild(m_pSwitch);
		m_pSwitch->addChild(m_pBoxMT);

		m_mWorldToBound = osg::computeWorldToLocal(m_pBoxMT->getParentalNodePaths(m_pRoot)[0]);
		m_bBound = m_pBoxMT->getBound();

	}

	osg::Material* makeMaterial(osg::Vec3f vColour)
	{
		osg::Material *pMat = new osg::Material();
		osg::Vec4f vAmb, vDiff, vSpe;

		vAmb.set(vColour[0] * 0.2f, vColour[1] * 0.2f, vColour[2] * 0.2f, 1.0f);
		vDiff.set(vColour[0] * 0.8f, vColour[1] * 0.8f, vColour[2] * 0.8f, 1.0f);
		vSpe.set(vColour[0] * 0.9f, vColour[1] * 0.9f, vColour[2] * 0.9f, 1.0f);

		pMat->setAmbient(osg::Material::FRONT_AND_BACK, vAmb);
		pMat->setDiffuse(osg::Material::FRONT_AND_BACK, vDiff);
		pMat->setSpecular(osg::Material::FRONT_AND_BACK, vSpe);

		return pMat;
	}

	osg::Vec3f animationPoint(unsigned int iIndex, osg::Node *pRoot)
	{
		return m_avAnimationPoints[iIndex] * osg::computeLocalToWorld(m_pRoot->getParentalNodePaths(pRoot)[0]);
	}

	virtual ~tileF()
	{
		if (m_pRoot) m_pRoot->unref();
	}

	void showDetector(bool bShow)
	{
		m_bShow = bShow;
		if (m_pSwitch)
		{
			if (bShow) m_pSwitch->setAllChildrenOn();
			else m_pSwitch->setAllChildrenOff();
		}
	}

	bool detectorVisible()
	{
		return m_bShow;
	}

	osg::Node* root()
	{
		return m_pRoot;
	}

	virtual bool run(osg::Object* object, osg::Object* data)
	{
		if(m_pRoot && g_Balls.size())
		{
			// test for entry to zone
			for(ballFs::iterator it=g_Balls.begin();it!=g_Balls.end();it++)
			{
				osg::Vec3f vPos(0.0f, 0.0f, 0.0f);
				vPos = vPos*osg::computeLocalToWorld((*it)->root()->getParentalNodePaths(m_pRoot)[0])*m_mWorldToBound;

				if (m_bBound.contains(vPos))
				{
					(*it)->root()->getOrCreateStateSet()->setAttributeAndModes(m_pOnMaterial, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
					m_Included.push_back((*it));
				}
			}

			//test for exit from zone
			if (m_Included.size())
			{
				ballFs::iterator it = m_Included.begin();
				while (it != m_Included.end())
				{
					osg::Vec3f vPos(0.0f, 0.0f, 0.0f);
					vPos = vPos*osg::computeLocalToWorld((*it)->root()->getParentalNodePaths(m_pRoot)[0])*m_mWorldToBound;

					if (!m_bBound.contains(vPos))
					{
						(*it)->root()->getOrCreateStateSet()->setAttributeAndModes(m_pOffMaterial, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
						ballFs::iterator it2 = it;
						it++;
						m_Included.remove((*it2));
						if (!m_Included.size()) break;
					}
					else
						it++;
				} 
			}

			if (m_Included.size()) m_pBoxMT->getOrCreateStateSet()->setAttributeAndModes(m_pOnMaterial, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
			else m_pBoxMT->getOrCreateStateSet()->setAttributeAndModes(m_pOffMaterial, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
		}
		return true;
	}

	protected:
	osg::MatrixTransform *m_pRoot;
	osg::MatrixTransform *m_pBoxMT;
	osg::Vec3f m_avAnimationPoints[4];

	osg::Material *m_pOffMaterial;
	osg::Material *m_pOnMaterial;

	osg::Switch *m_pSwitch;
	bool m_bShow;

	osg::Matrix m_mWorldToBound;
	osg::BoundingSphere m_bBound;
	ballFs m_Included;
};
#endif

typedef std::map<unsigned int, tileF*> tileFs;
tileFs g_Tiles;
osg::Node *g_pTile = 0;

void init(int argc, char** argv)
{
	g_pRoot = new osg::Group();
	g_pRoot->ref();

	osg::Group *pTileGroup = new osg::Group();
	osg::Geode *pTileGeode = new osg::Geode();
	osg::ShapeDrawable *pTileShape = new osg::ShapeDrawable(new osg::Box(osg::Vec3f(0.0f, 0.0f, 0.0f), 100.0f, 100.0f, 1.0f));

	pTileGeode->addDrawable(pTileShape);
	pTileGroup->addChild(pTileGeode);
	g_pTile = pTileGroup;

	g_Tiles[0] = new tileF(g_pTile, 0.0f, 0.0f, osg::Vec3f(0.6f, 0.0f, 0.0f));
	g_Tiles[1] = new tileF(g_pTile, 100.0f, 0.0f, osg::Vec3f(0.0f, 0.6f, 0.0f));
	g_Tiles[2] = new tileF(g_pTile, 200.0f, 0.0f, osg::Vec3f(0.0f, 0.0f, 0.6f));
	g_Tiles[3] = new tileF(g_pTile, 200.0f, 100.0f, osg::Vec3f(0.6f, 0.6f, 0.0f));

	for (tileFs::iterator it = g_Tiles.begin(); it != g_Tiles.end(); it++) g_pRoot->addChild(it->second->root());

#ifndef example1
	osg::AnimationPath *pPath = new osg::AnimationPath();
	// insert - timestep in seconds, selected animation point)
	// time gap needs calculating dependent on distance otherwise animated gemoetry will speed up / slow down over 
	// different animation point distances
	// project animation points are cones so you can see direction
	const float distance = 100.0f;
	float fTime = 0.0f;
	pPath->insert(fTime, osg::AnimationPath::ControlPoint(g_Tiles[3]->animationPoint(0, g_pRoot)));
	pPath->insert(fTime += 1.0f, osg::AnimationPath::ControlPoint(g_Tiles[2]->animationPoint(0, g_pRoot)));
	pPath->insert(fTime += 1.0f, osg::AnimationPath::ControlPoint(g_Tiles[1]->animationPoint(0, g_pRoot)));
	pPath->insert(fTime += 1.0f, osg::AnimationPath::ControlPoint(g_Tiles[0]->animationPoint(0, g_pRoot)));
	pPath->insert(fTime += (40.0f / distance), osg::AnimationPath::ControlPoint(g_Tiles[0]->animationPoint(1, g_pRoot)));
	pPath->insert(fTime += 1.0f, osg::AnimationPath::ControlPoint(g_Tiles[1]->animationPoint(1, g_pRoot)));
	pPath->insert(fTime += 1.0f, osg::AnimationPath::ControlPoint(g_Tiles[2]->animationPoint(1, g_pRoot)));
	pPath->insert(fTime += (60.0f / distance), osg::AnimationPath::ControlPoint(g_Tiles[3]->animationPoint(0, g_pRoot)));
	g_Balls.push_back(new ballF(pPath));

#ifdef example4
	osg::AnimationPath *pPath2 = new osg::AnimationPath();
	pPath2->insert(0.0f, osg::AnimationPath::ControlPoint(g_Tiles[1]->animationPoint(0, g_pRoot)));
	pPath2->insert(1.0f, osg::AnimationPath::ControlPoint(g_Tiles[3]->animationPoint(1, g_pRoot)));
	pPath2->insert(2.0f, osg::AnimationPath::ControlPoint(g_Tiles[2]->animationPoint(2, g_pRoot)));
	pPath2->insert(3.0f, osg::AnimationPath::ControlPoint(g_Tiles[0]->animationPoint(3, g_pRoot)));
	g_Balls.push_back(new ballF(pPath2));
#endif

	for (ballFs::iterator it = g_Balls.begin(); it != g_Balls.end(); it++) g_pRoot->addChild((*it)->root());

#endif
}


class raaEventHandler : public osgGA::GUIEventHandler
{
public:
	raaEventHandler() {};
	virtual ~raaEventHandler() {};

	virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
	{
		if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
		{
			switch (ea.getKey())
			{
			case 'p':
			case 'P':
			{
				nodePrinter printer;
				printer.traverse(*g_pRoot);
			}
			return true;
#ifdef example4
			// only for showoff case
			case 'b':
			case 'B':
				for(tileFs::iterator it=g_Tiles.begin();it!=g_Tiles.end();it++)
				{
					it->second->showDetector(!it->second->detectorVisible());
				}

			return true;;
#endif
			}
		}
		return false;
	}
};

int main(int argc, char** argv)
{
	init(argc, argv);

	osgViewer::Viewer viewer;

	osg::GraphicsContext::Traits *pTraits = new osg::GraphicsContext::Traits();
	pTraits->x = 20;
	pTraits->y = 20;
	pTraits->width = 600;
	pTraits->height = 480;
	pTraits->windowDecoration = true;
	pTraits->doubleBuffer = true;
	pTraits->sharedContext = 0;

	osg::GraphicsContext *pGC = osg::GraphicsContext::createGraphicsContext(pTraits);
	osgGA::KeySwitchMatrixManipulator* pKeyswitchManipulator = new osgGA::KeySwitchMatrixManipulator();
	pKeyswitchManipulator->addMatrixManipulator('1', "Trackball", new osgGA::TrackballManipulator());
	pKeyswitchManipulator->addMatrixManipulator('2', "Flight", new osgGA::FlightManipulator());
	pKeyswitchManipulator->addMatrixManipulator('3', "Drive", new osgGA::DriveManipulator());
	viewer.setCameraManipulator(pKeyswitchManipulator);
	osg::Camera *pCamera = viewer.getCamera();
	pCamera->setGraphicsContext(pGC);
	pCamera->setViewport(new osg::Viewport(0, 0, pTraits->width, pTraits->height));

	// add own event handler
	viewer.addEventHandler(new raaEventHandler());

	// add the state manipulator
	viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));

	// add the thread model handler
	viewer.addEventHandler(new osgViewer::ThreadingHandler);

	// add the window size toggle handler
	viewer.addEventHandler(new osgViewer::WindowSizeHandler);

	// add the stats handler
	viewer.addEventHandler(new osgViewer::StatsHandler);

	// add the record camera path handler
	viewer.addEventHandler(new osgViewer::RecordCameraPathHandler);

	// add the LOD Scale handler
	viewer.addEventHandler(new osgViewer::LODScaleHandler);

	// add the screen capture handler
	viewer.addEventHandler(new osgViewer::ScreenCaptureHandler);

	// set the scene to render
	viewer.setSceneData(g_pRoot);

	viewer.realize();

	return viewer.run();
}


