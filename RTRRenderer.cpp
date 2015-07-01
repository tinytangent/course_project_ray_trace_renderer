#include "RTRRenderer.h"
#include <QColor>
#include <QDebug>
#include <QImage>
#include <time.h>
#include "RTRGeometry.h"
#include "Light/RTRLightPoint.h"

RTRRenderer::RTRRenderer(QImage *_image)
{
	model = NULL;
	camera = NULL;
	image = _image;
	//zBuffer = new double[image->width()*image->height()];
}

void RTRRenderer::render()
{
	int beginTime = clock();
	//初始化深度缓冲，由于即将切换至光线追踪算法，下面的代码将不再被需要
	/*for(int i=0;i<image->width();i++)
	{
		for(int j=0;j<image->height();j++)
		{
			zBuffer[i*image->height()+j] = 1e50;
		}
	}*/

	if(model == NULL || camera == NULL) return;

	//扫描所有需要渲染的多边形
	foreach(const RTRModelPolygen* face, model->polygens)
	{
		//if (face->objectName != "Plane") continue;
		//TODO : 只适用于凸多边形！
		//三角形的三个顶点
		RTRVector point1(3), point2(3), point3(3);

		//将多边形的第一个顶点作为三角形的第一个顶m点
		point1 = face->vertices[0]->position;

		//而三角形的另外两个顶点从多边形剩下的顶点中取两两相邻的选取。
		for (int j = 1; j < face->vertices.size() - 1; j++)
		{
			//构建新的三角形
			point2 = face->vertices[j]->position;
			point3 = face->vertices[j + 1]->position;

			QColor myBlue;
			myBlue.setRedF(0.5);
			myBlue.setGreenF(0.5);
			myBlue.setBlueF(0.5);

			RTRMaterial material(myBlue,Qt::black);


			//TODO: 存在内存泄漏问题！
			RTRRenderElement* element = new RTRRenderElement(new RTRTriangle3D(point1, point2, point3), camera);
			element->objectName = face -> objectName;
			element->material = model->materialLibrary[face->materialName];
			if (face->uvMaps.size() > 0)
			{
				element->vertexUVMaps[0] = face->uvMaps[0];
				element->vertexUVMaps[1] = face->uvMaps[j];
				element->vertexUVMaps[2] = face->uvMaps[j + 1];
			}
			//element->material = &material;
			/*if (model->vertexNormals.size() > face.normals[j + 1] - 1 &&
				model->vertexNormals.size() > face.normals[j] - 1 &&
				model->vertexNormals.size() > face.normals[0] - 1)
			{
				element->useSmoothShading = true;
				element->vertexNormals[0] = model->vertexNormals[face.normals[0] - 1];
				element->vertexNormals[1] = model->vertexNormals[face.normals[j] - 1];
				element->vertexNormals[2] = model->vertexNormals[face.normals[j + 1] - 1];
			}*/
			//else
			{
				element->useSmoothShading = false;
			}
			/*if (face.materialName == "") element->material = NULL;
			else
			{
				element->material = model->materialLibrary[face.materialName];
			}*/

			//将三角形添加到需要渲染的三角形的列表之中
			elements.append(element);
		}
	}
	elementsCache = RTRKdTree::create(elements);

	//遍历所有的像素点，从它们发出光线并计算渲染结果。
	for(int i=0;i<image->width();i++)
	{
		for(int j=0;j<image->height();j++)
		{
			RTRRay ray = RTRGeometry::invertProject(RTRVector2D(i,j),*camera);
			RTRColor color = renderRay(ray);
			renderPixel(i,j,0,color);
		}
	}
	int endTime = clock();
	qDebug() << ((float)endTime - beginTime)/CLOCKS_PER_SEC;
}

void RTRRenderer::renderLineByDDA(int x1, int y1, int x2, int y2, const QColor &color)
{
	//x1 += 300;
	//x2 += 300;
	//y1 += 300;
	//y2 += 300;
	//This improved DDA Alogrithm handles x1/x2 in reverse order,
	//or \Delta Y>\Delta X cases.
	//Note: about the 0.5 problem: using qRound Function
	int deltaX = qAbs(x1 - x2);
	int deltaY = qAbs(y1 - y2);
	if (deltaX > deltaY)
	{
		if(x1 > x2)
		{
			qSwap(x1,x2);
			qSwap(y1,y2);
		}
		double slope = ((double)(y2-y1))/(x2-x1);
		double y = y1;
		for(int x = x1; x <= x2; x++, y += slope)
		{
			if(x>=0 && x<image->width() && y>=0 && y<image->height())
				image->setPixel(x, qRound(y), color.rgb());
		}
	}
	else
	{
		if(y1 > y2)
		{
			qSwap(x1,x2);
			qSwap(y1,y2);
		}
		double slope = ((double)(x2-x1))/(y2-y1);
		double x = x1;
		for(int y = y1; y <= y2; y++, x += slope)
		{
			if(x>=0 && x<image->width() && y>=0 && y<image->height())
				image->setPixel(qRound(x), y, color.rgb());
		}
	}
}

void RTRRenderer::renderLineByDDA(RTRVector2D p1, RTRVector2D p2, const QColor &color)
{
	renderLineByDDA(p1.x(), p1.y(), p2.x(), p2.y(), color);
}


RTRColor RTRRenderer::renderRay(const RTRRay& ray, int iterationCount, const RTRRenderElement* elementFrom)
{
	//RTRVector* vec3D = new RTRVector(3)[100];
	//RTRLightPoint lightPoint(RTRVector(4.07625,1.00545,5.90386),RTRColor(1,1,1),7.5);
	RTRLightPoint lightPoint(RTRVector(2.3, -0.9, 6.7), RTRColor(1, 1, 1), 7.5);
	QColor myBlue;
	myBlue.setRedF(0.5);
	myBlue.setGreenF(0.5);
	myBlue.setBlueF(0.5);
	RTRMaterial material(myBlue,Qt::black);

	//QSet<RTRRenderElement*> possibleElements;
	RTRRenderElement* possibleElements;
	elementsCache->search(possibleElements, ray, elementFrom);

	//TODO: check 3D Z Order Algorithm

	RTRColor diffuseColor(0.0, 0.0, 0.0);
	double minZ = 1e50;
	const RTRRenderElement* frontElement = NULL;

	RTRVector3D intersectPoint(0.0, 0.0, 0.0);
	RTRVector3D intersectNormal(0.0, 0.0, 0.0);
	RTRColor intersectColor(0.0, 0.0, 0.0);
	//foreach(const RTRRenderElement* element, possibleElements)
	if (possibleElements != NULL)
	{
		RTRRenderElement* element = possibleElements;
		RTRVector3D point(0.0, 0.0, 0.0);
		RTRVector3D normal(0.0, 0.0, 0.0);
		RTRColor objColor(0.0, 0.0, 0.0);
		element->intersect(ray, point, normal, objColor);
		//if (!element->intersect(ray, point, normal, objColor));continue;
		double zValue = ray.direction.dotProduct(point - ray.beginningPoint);
		//if (zValue < 0) continue;
		//double zValue = abs(point.z() - ray.beginningPoint.z());
		//if (zValue > minZ) continue;
		//if (element == elementFrom) continue;
		frontElement = element;
		intersectPoint = point;
		intersectColor = objColor;
		intersectNormal = normal;
		minZ = zValue;
		RTRVector v =lightPoint.directionAt(point);
		double decay = lightPoint.directionAt(point).dotProduct(normal);
		int sym1 = sgn(decay);
		int sym2 = sgn((point - ray.beginningPoint).dotProduct(normal));
		decay = decay>0?decay:-decay;
		if(sym1 == sym2)
		{
			RTRColor lightColor = lightPoint.colorAt(point);
			diffuseColor.r() = objColor.r()*lightColor.r()*decay;
			diffuseColor.g() = objColor.g()*lightColor.g()*decay;
			diffuseColor.b() = objColor.b()*lightColor.b()*decay;
		}
		else
		{
			diffuseColor.r() = 0.0;
			diffuseColor.g() = 0.0;
			diffuseColor.b() = 0.0;
		}
	}

	RTRColor reflectionColor(1.0,0.0,0.0);
	if (frontElement != NULL && frontElement->objectName == "Plane" /*&& (frontElement->objectName == "Sphere" || frontElement->objectName == "Cube_Cube.001")*/ && iterationCount<3)
	{
		RTRVector3D reflectionDirection(0.0, 0.0, 0.0);
		reflectionDirection = (intersectNormal * 2 * ray.direction.dotProduct(intersectNormal) - ray.direction)*-1;
		RTRRay reflectionRay(intersectPoint,reflectionDirection,RTRRay::CREATE_FROM_POINT_AND_DIRECTION);
		reflectionColor = renderRay(reflectionRay, iterationCount + 1, frontElement);
		return reflectionColor*0.5 + diffuseColor*0.5;
	}
	else
	{
		return diffuseColor;
	}
}

void RTRRenderer::renderPixel(int x, int y, double z, const RTRColor& color)
{
	image->setPixel(x,image->height()-y,color.toQtColor().rgb());
}