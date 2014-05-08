//
//  CAScrollView.cpp
//  cocos2dx
//
//  Created by Li Yuanfeng on 14-4-23.
//  Copyright (c) 2014 www.9miao.com All rights reserved.
//

#include "CAScrollView.h"
#include "sprite_nodes/CCScale9Sprite.h"
#include "actions/CCActionInterval.h"
#include "actions/CCActionEase.h"

NS_CC_BEGIN

#pragma CAScrollView

CAScrollView::CAScrollView()
:m_pContainer(NULL)
,m_obViewSize(CCSizeZero)
,m_pScrollViewDelegate(NULL)
,m_bBounces(true)
,m_bBounceHorizontal(true)
,m_bBounceVertical(true)
,m_bscrollEnabled(true)
,m_bTracking(false)
,m_bDecelerating(false)
,m_bBouncesZoom(true)
,m_bZooming(false)
,m_fMaximumZoomScale(1.0f)
,m_fMinimumZoomScale(1.0f)
,m_fZoomScale(1.0f)
,m_fTouchLength(0.0f)
,m_tInertia(CCPointZero)
,m_bShowsHorizontalScrollIndicator(true)
,m_bShowsVerticalScrollIndicator(true)
{
    m_pTouches = new CCArray(2);
    m_pChildInThis = new CCArray();
}

CAScrollView::~CAScrollView()
{
    CC_SAFE_DELETE(m_pTouches);
    CC_SAFE_DELETE(m_pChildInThis);
}

void CAScrollView::onEnterTransitionDidFinish()
{
    CAView::onEnterTransitionDidFinish();
    
    this->scheduleUpdate();
}

void CAScrollView::onExitTransitionDidStart()
{
    CAView::onExitTransitionDidStart();
    
    m_tPointOffset.clear();
    m_pTouches->removeAllObjects();
    m_tInertia = CCPointZero;
}

bool CAScrollView::initWithFrame(const cocos2d::CCRect &rect)
{
    if (!CAView::initWithFrame(rect, ccc4(255, 255, 255, 0)))
    {
        return false;
    }
    
    this->setTouchEnabled(true);
    this->setDisplayRange(false);
    
    m_pContainer = CAView::createWithFrame(this->getBounds(), ccc4(255, 255, 255, 0));
    m_pChildInThis->addObject(m_pContainer);
    this->addChild(m_pContainer);
    
    m_pIndicatorHorizontal = CAIndicator::createWithFrame(CCRect(12, 2, rect.size.width - 24, 10), CAIndicator::CAIndicatorTypeHorizontal);
    m_pChildInThis->addObject(m_pIndicatorHorizontal);
    this->addChild(m_pIndicatorHorizontal, 1);
    
    m_pIndicatorVertical = CAIndicator::createWithFrame(CCRect(rect.size.width - 12, 12, 10, rect.size.height - 24), CAIndicator::CAIndicatorTypeVertical);
    m_pChildInThis->addObject(m_pIndicatorVertical);
    this->addChild(m_pIndicatorVertical, 1);
    
    
    return true;
}

void CAScrollView::addChild(CCNode * child)
{
    this->addChild(child, 0);
}

void CAScrollView::addChild(CCNode * child, int zOrder)
{
    this->addChild(child, zOrder, -1);
}

void CAScrollView::addChild(CCNode* child, int zOrder, int tag)
{
    do
    {
        CC_BREAK_IF(m_pChildInThis->containsObject(child));
        m_pContainer->addChild(child, zOrder, tag);
        return;
    }
    while (0);
    
    CAView::addChild(child, zOrder, tag);
}

void CAScrollView::removeAllChildrenWithCleanup(bool cleanup)
{
    m_pContainer->removeAllChildrenWithCleanup(cleanup);
}

void CAScrollView::setViewSize(cocos2d::CCSize var)
{
    do
    {
        CC_BREAK_IF(m_obViewSize.equals(var));
        
        m_obViewSize = var;
        
        CC_BREAK_IF(m_pContainer == NULL);
        
        m_pContainer->setBoundsSize(m_obViewSize);
        
        CCRect rect = CCRectZero;
        rect.size = m_pContainer->getBounds().size;
        rect.origin.y = m_obContentSize.height - rect.size.height;
        m_pContainer->setFrame(rect);
        
    }
    while (0);
    
}

CCSize CAScrollView::getViewSize()
{
    return m_obViewSize;
}

void CAScrollView::setBounces(bool var)
{
    m_bBounces = var;
    if (var == false)
    {
        m_bBounceHorizontal = false;
        m_bBounceVertical = false;
    }
}

bool CAScrollView::isBounces()
{
    return m_bBounces;
}

void CAScrollView::setShowsHorizontalScrollIndicator(bool var)
{
    m_bShowsHorizontalScrollIndicator = var;
    if (m_bShowsHorizontalScrollIndicator)
    {
        m_pIndicatorHorizontal->setVisible(true);
    }
    else
    {
        m_pIndicatorHorizontal->setVisible(false);
    }
}

bool CAScrollView::isShowsHorizontalScrollIndicator()
{
    return m_bShowsHorizontalScrollIndicator;
}

void CAScrollView::setShowsVerticalScrollIndicator(bool var)
{
    m_bShowsVerticalScrollIndicator = var;
    if (m_bShowsVerticalScrollIndicator)
    {
        m_pIndicatorVertical->setVisible(true);
    }
    else
    {
        m_pIndicatorVertical->setVisible(false);
    }
}

bool CAScrollView::isShowsVerticalScrollIndicator()
{
    return m_bShowsVerticalScrollIndicator;
}

bool CAScrollView::ccTouchBegan(CCTouch *pTouch, CCEvent *pEvent)
{
    do
    {
        
        CCPoint point = this->convertTouchToNodeSpace(pTouch);
        CC_BREAK_IF(m_bscrollEnabled == false);
        CC_BREAK_IF(!this->isVisible());
        CC_BREAK_IF(!getBounds().containsPoint(point));
        CC_BREAK_IF(m_pTouches->count() > 2);

        if (!m_pTouches->containsObject(pTouch))
        {
            m_pTouches->addObject(pTouch);
        }
        
        if (m_pTouches->count() == 1)
        {
            this->unschedule(schedule_selector(CAScrollView::deaccelerateScrolling));
            m_tInertia = CCPointZero;
        }
        else if (m_pTouches->count() == 2)
        {
            CCTouch* touch0 = dynamic_cast<CCTouch*>(m_pTouches->objectAtIndex(0));
            CCTouch* touch1 = dynamic_cast<CCTouch*>(m_pTouches->objectAtIndex(1));
           
            m_fTouchLength = ccpDistance(touch0->getLocation(), touch1->getLocation());
            
            CCPoint mid_point = ccpMidpoint(touch0->getLocation(), touch1->getLocation());
            
            CCPoint p = m_pContainer->convertToNodeSpace(mid_point);
            p = CCPoint(p.x / m_pContainer->getContentSize().width,
                        p.y / m_pContainer->getContentSize().height);
            m_pContainer->setAnchorPoint(p);
            m_pContainer->setPosition(this->convertToNodeSpace(mid_point));
            
            if (m_pScrollViewDelegate)
            {
                m_pScrollViewDelegate->scrollViewDidZoom(this);
            }
            m_bZooming = true;
        }
        
        return true;
    }
    while (0);

    return false;
}

void CAScrollView::ccTouchMoved(CCTouch *pTouch, CCEvent *pEvent)
{
    CCPoint p_container = m_pContainer->getCenter();
    
    if (m_pTouches->count() == 1)
    {
        CCPoint p_off = CCPointZero;
        p_off = ccpSub(pTouch->getLocation(), pTouch->getPreviousLocation());
        
		if (m_bBounces)
		{
			CCSize size = this->getContentSize();
			CCRect rect = m_pContainer->getFrame();

			if (p_container.x - rect.size.width / 2 > 0)
			{
				p_off.x *= 0.618f - rect.origin.x / size.width;
			}

			if (p_container.y - rect.size.height / 2 > 0)
			{
				p_off.y *= 0.618f - rect.origin.y / size.height;
			}

			if ((p_container.x + rect.size.width / 2 - size.width) < 0)
			{
				p_off.x *= (rect.size.width + rect.origin.x) / size.width - 1 + 0.618f;
			}

			if ((p_container.y + rect.size.height / 2 - size.height) < 0)
			{
				p_off.y *= (rect.size.height + rect.origin.y) / size.height - 1 + 0.618f;
			}
		}

		p_container = ccpAdd(p_container, p_off);

		m_tPointOffset.push_back(p_off);

		if (m_tPointOffset.size() > 3)
		{
			m_tPointOffset.pop_front();
		}

        if (m_pScrollViewDelegate)
        {
            m_pScrollViewDelegate->scrollViewWillBeginDragging(this);
        }
        m_bTracking = true;
    }
    else if (m_pTouches->count() == 2)
    {
        CCTouch* touch0 = dynamic_cast<CCTouch*>(m_pTouches->objectAtIndex(0));
        CCTouch* touch1 = dynamic_cast<CCTouch*>(m_pTouches->objectAtIndex(1));
        
        if (m_fMinimumZoomScale < m_fMaximumZoomScale)
        {
            float touch_lenght = ccpDistance(touch0->getLocation(), touch1->getLocation());
            float scale_off = touch_lenght - m_fTouchLength;
            
            m_fZoomScale = m_pContainer->getScale();
            m_fZoomScale += scale_off * 0.002f;
            
            if (!m_bBouncesZoom)
            {
                m_fZoomScale = MIN(m_fZoomScale, m_fMaximumZoomScale);
                m_fZoomScale = MAX(m_fZoomScale, m_fMinimumZoomScale);
            }
            
            m_pContainer->setScale(m_fZoomScale);
            m_fTouchLength = touch_lenght;
        }
        
		CCPoint mid_point = ccpMidpoint(touch0->getLocation(), touch1->getLocation());

		m_pContainer->setPosition(this->convertToNodeSpace(mid_point));
        
        p_container = m_pContainer->getCenter();
    }
    
    
    if (m_bBounces == false)
    {
        p_container = this->getScrollWindowNotOutPoint(p_container);
    }
    else
    {
        if (m_bBounceHorizontal == false)
        {
            p_container.x = this->getScrollWindowNotOutHorizontal(p_container.x);
        }
        
        if (m_bBounceVertical == false)
        {
            p_container.y = this->getScrollWindowNotOutVertical(p_container.y);
        }
    }
    
    if (m_pScrollViewDelegate && p_container.equals(m_pContainer->getCenter()) == false)
    {
        m_pScrollViewDelegate->scrollViewDidScroll(this);
    }
    
    m_pContainer->setCenter(p_container);
}

void CAScrollView::ccTouchEnded(CCTouch *pTouch, CCEvent *pEvent)
{
    if (m_pTouches->containsObject(pTouch))
    {
        if (m_pTouches->count() == 1)
        {
            CCPoint p = CCPointZero;
            
            if (m_tPointOffset.size() > 0)
            {
//                if (m_tPointOffset.size() > 1)
//                {
//                    m_tPointOffset.pop_back();
//                }
                
                for (unsigned int i=0; i<m_tPointOffset.size(); i++)
                {
                    p = ccpAdd(p, m_tPointOffset.at(i));
                }
                p = p/m_tPointOffset.size();
            }
            m_tInertia = p;
			m_tPointOffset.clear();

            if (m_pScrollViewDelegate)
            {
                m_pScrollViewDelegate->scrollViewDidEndDragging(this);
            }
            m_bTracking = false;
            
            this->schedule(schedule_selector(CAScrollView::deaccelerateScrolling));

        }
        else if (m_pTouches->count() == 2)
        {
            CCPoint point = m_pContainer->getCenter();
            m_pContainer->setPosition(point);
            m_pContainer->setAnchorPoint(CCPoint(0.5f, 0.5f));

            m_bZooming = false;
        }
        m_pTouches->removeObject(pTouch);
    }
}

void CAScrollView::ccTouchCancelled(CCTouch *pTouch, CCEvent *pEvent)
{
    this->ccTouchEnded(pTouch, pEvent);
}

void CAScrollView::deaccelerateScrolling(float delay)
{
    if (m_tInertia.getLength() > maxSpeedCache())
    {
        m_tInertia = ccpMult(m_tInertia, maxSpeedCache() / m_tInertia.getLength());
    }
    
    CCPoint speed;
    if (m_tInertia.getLength() > maxSpeed())
    {
        speed = ccpMult(m_tInertia, maxSpeed() / m_tInertia.getLength());
    }
    else
    {
        speed = m_tInertia;
    }
    
    CCPoint point = m_pContainer->getCenter();

    if (m_bBounces)
    {
        CCSize size = this->getContentSize();
        CCRect rect = m_pContainer->getFrame();
        
        CCPoint resilience = CCPointZero;
        
        if (point.x - rect.size.width / 2 > 0)
        {
            resilience.x = (point.x - rect.size.width / 2) / size.width;
        }
        
        if (point.y - rect.size.height / 2 > 0)
        {
            resilience.y = (point.y - rect.size.height / 2) / size.height;
        }
        
        if ((point.x + rect.size.width / 2 - size.width) < 0)
        {
            resilience.x = (point.x + rect.size.width / 2 - size.width) / size.width;
        }
        
        if ((point.y + rect.size.height / 2 - size.height) < 0)
        {
            resilience.y = (point.y + rect.size.height / 2 - size.height) / size.height;
        }
        
        resilience = ccpMult(resilience, maxBouncesSpeed());
        
        speed = ccpSub(speed, resilience);
    }
    
    point = ccpAdd(point, speed);
    
    if (m_bBounces == false)
    {
        point = this->getScrollWindowNotOutPoint(point);
    }
    else
    {
        if (m_bBounceHorizontal == false)
        {
            point.x = this->getScrollWindowNotOutHorizontal(point.x);
        }
        
        if (m_bBounceVertical == false)
        {
            point.y = this->getScrollWindowNotOutVertical(point.y);
        }
    }
    
    if (m_pScrollViewDelegate && point.equals(m_pContainer->getCenter()) == false)
    {
        m_pScrollViewDelegate->scrollViewDidScroll(this);
    }
    
    m_pContainer->setCenter(point);
    
    m_tInertia = ccpMult(m_tInertia, 1 - decelerationRatio());
    
    m_bDecelerating = true;
    
    if (speed.getLength() < 0.001f)
    {
        m_bDecelerating = false;
        this->unschedule(schedule_selector(CAScrollView::deaccelerateScrolling));
    }
}

float CAScrollView::maxSpeed()
{
    return (CCPoint(m_obContentSize).getLength() / 12);
}

float CAScrollView::maxSpeedCache()
{
    return (maxSpeed() * 1.5f);
}

float CAScrollView::decelerationRatio()
{
    return 0.25f;
}

float CAScrollView::maxBouncesSpeed()
{
    return (CCPoint(m_obContentSize).getLength() / 10);
}

void CAScrollView::update(float fDelta)
{
    m_pIndicatorHorizontal->setIndicator(m_obContentSize, m_pContainer->getFrame());
    m_pIndicatorVertical->setIndicator(m_obContentSize, m_pContainer->getFrame());
}

const CCPoint& CAScrollView::getScrollWindowNotOutPoint(CCPoint& point)
{
    point.x = this->getScrollWindowNotOutHorizontal(point.x);
    point.y = this->getScrollWindowNotOutVertical(point.y);
    
    return point;
}

float CAScrollView::getScrollWindowNotOutHorizontal(float x)
{
    CCSize size = this->getContentSize();
    CCRect rect = m_pContainer->getFrame();
    
    if (x - rect.size.width / 2 > 0)
    {
        x = rect.size.width / 2;
    }
    
    if ((x + rect.size.width / 2 - size.width) < 0)
    {
        x = size.width - rect.size.width / 2;
    }
    
    return x;
}

float CAScrollView::getScrollWindowNotOutVertical(float y)
{
    CCSize size = this->getContentSize();
    CCRect rect = m_pContainer->getFrame();
    
    if (y - rect.size.height / 2 > 0)
    {
        y = rect.size.height / 2;
    }
    
    if ((y + rect.size.height / 2 - size.height) < 0)
    {
        y = size.height - rect.size.height / 2;
    }
    
    return y;
}

bool CAScrollView::isScrollWindowNotOutSide()
{
    CCSize size = this->getContentSize();
    CCRect rect = m_pContainer->getFrame();
    CCPoint point = m_pContainer->getCenter();
    
    if (point.x - rect.size.width / 2 > 0.5f)
    {
        return true;
    }
    
    if ((point.x + rect.size.width / 2 - size.width) < -0.5f)
    {
        return true;
    }
    
    if (point.y - rect.size.height / 2 > 0.5f)
    {
        return true;
    }
    
    if ((point.y + rect.size.height / 2 - size.height) < -0.5f)
    {
        return true;
    }
    
    return false;
}

#pragma CAIndicator

CAIndicator::CAIndicator()
:m_pIndicator(NULL)
{

}

CAIndicator::~CAIndicator()
{

}

CAIndicator* CAIndicator::createWithFrame(const CCRect& rect, CAIndicatorType type)
{
    CAIndicator* indicator = new CAIndicator();
    if (indicator && indicator->initWithFrame(rect, type))
    {
        indicator->autorelease();
        return indicator;
    }
    CC_SAFE_DELETE(indicator);
    return NULL;
}

bool CAIndicator::initWithFrame(const CCRect& rect, CAIndicatorType type)
{
    if (!CCNodeRGBA::init())
    {
        return false;
    }
    this->setFrame(rect);
    
    m_eType = type;
    
    CCTexture2D* texture = CCTexture2D::create("indicator.png");
    
    CCRect r;
    r.origin = ccpSub(ccpMult(texture->getContentSize(), 0.5f), CCPoint(0.5f, 0.5f));
    r.size = CCSize(1.0f, 1.0f);
    
    m_pIndicator = CCScale9Sprite::createWithTexture(r, texture);
    this->addChild(m_pIndicator);
    
    return true;
}

void CAIndicator::setIndicator(const CCSize& parentSize, const CCRect& childrenFrame)
{
    CCScale9Sprite* indicator = dynamic_cast<CCScale9Sprite*>(m_pIndicator);
    
    int x = (int)indicator->getPositionX() * 10;
    int y = (int)indicator->getPositionY() * 10;
    
    
    if (m_eType == CAIndicatorTypeHorizontal)
    {
        float size_scale_x = parentSize.width / childrenFrame.size.width;
        size_scale_x = MIN(size_scale_x, 1.0f);
        
        float lenght_scale_x = parentSize.width / 2 - childrenFrame.origin.x;
        lenght_scale_x /= childrenFrame.size.width;
        
        CCSize size = m_obContentSize;
        size.width *= size_scale_x;
        
        if (lenght_scale_x < size_scale_x / 2)
        {
            size.width *= lenght_scale_x / (size_scale_x / 2);
        }
        if (1 - lenght_scale_x < size_scale_x / 2)
        {
            size.width *= (1 - lenght_scale_x) / (size_scale_x / 2);
        }
        size.width = MAX(size.width, 0);
        indicator->setPreferredSize(size);
        
        
        CCPoint point = m_obContentSize;
        point.y *= 0.5f;
        point.x *= lenght_scale_x;
        point.x = MAX(point.x, size.width/2);
        point.x = MIN(point.x, m_obContentSize.width - size.width/2);
        indicator->setPosition(point);
    }
    else if (m_eType == CAIndicatorTypeVertical)
    {
        float size_scale_y = parentSize.height / childrenFrame.size.height;
        size_scale_y = MIN(size_scale_y, 1.0f);
        
        float lenght_scale_y = parentSize.height / 2 - childrenFrame.origin.y;
        lenght_scale_y /= childrenFrame.size.height;
        
        CCSize size = m_obContentSize;
        size.height *= size_scale_y;
        
        if (lenght_scale_y < size_scale_y / 2)
        {
            size.height *= lenght_scale_y / (size_scale_y / 2);
        }
        if (1 - lenght_scale_y < size_scale_y / 2)
        {
            size.height *= (1 - lenght_scale_y) / (size_scale_y / 2);
        }
        size.height = MAX(size.height, size.width);
        indicator->setPreferredSize(size);
        
        
        CCPoint point = m_obContentSize;
        point.x *= 0.5f;
        point.y *= lenght_scale_y;
        point.y = MAX(point.y, size.height/2);
        point.y = MIN(point.y, m_obContentSize.height - size.height/2);
        
        indicator->setPosition(point);
    }
    
    do
    {
        if (x == (int)indicator->getPositionX() * 10 && y == (int)indicator->getPositionY() * 10)
        {
            CC_BREAK_IF(indicator->getActionByTag(0xfff));
            
            CC_BREAK_IF(indicator->getOpacity() < 255);
            
            CCDelayTime* delayTime = CCDelayTime::create(0.2f);
            CCFadeOut* fadeOut = CCFadeOut::create(0.3f);
            CCEaseSineOut* easeSineOut = CCEaseSineOut::create(fadeOut);
            CCSequence* actions = CCSequence::create(delayTime, easeSineOut, NULL);
            actions->setTag(0xfff);
            indicator->runAction(actions);
        }
        else
        {
            CC_BREAK_IF(indicator->getOpacity() == 255);
            
            indicator->stopAllActions();
            indicator->setOpacity(255);
        }
    }
    while (0);
}

void CAIndicator::setOpacity(GLubyte opacity)
{
    CCScale9Sprite* indicator = dynamic_cast<CCScale9Sprite*>(m_pIndicator);
    indicator->setOpacity(opacity);
    indicator->setColor(ccc3(opacity, opacity, opacity));
}

#pragma CAScrollViewDelegate

void CAScrollViewDelegate::removeDelegate(CAScrollView* view)
{
    view->setScrollViewDelegate(NULL);
}

NS_CC_END