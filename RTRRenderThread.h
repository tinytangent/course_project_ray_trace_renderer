#ifndef RTRRENDERTHREAD_H
#define RTRRENDERTHREAD_H

#include <QThread>
#include "RTRRenderer.h"

class RTRRenderThread : public QThread
{
	Q_OBJECT
public:
	int xBegin;
	int xEnd;
	int yBegin;
	int yEnd;
	int threadIndex;
	RTRRenderer* renderer;
	RTRRenderThread(RTRRenderer* _renderer, int _threadIndex);
	RTRColor* renderResult;

public slots:
	void start(int _xBegin, int _xEnd, int _yBegin, int _yEnd);

protected:
	virtual void run() Q_DECL_OVERRIDE;
	/**
	* @brief renderRayʵ���˹���׷�ٵĺ����㷨��������Ⱦһ����·�ġ���ɫ����
	* @param ray��Ҫ����׷�ٵĹ���
	* @param iterationCount����������Ϊ������ã�Ĭ�ϲ���ֵΪ0
	* @param elementFrom ������������ߵ���ȾԪ�ء��ò�����Ҫ���ڷ�ֹ��������������Ϊ���Լ��뽻�������
	* @return ��õ���ɫ��ֵ��
	*/
	RTRColor renderRay(const RTRRay& ray, int iterationCount = 0, const RTRRenderElement* elementFrom = NULL, double refracInAir = true);

signals:
	void renderFinished(int _threadIndex);
};

#endif