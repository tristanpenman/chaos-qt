#pragma once

#include <QColor>
#include <QGraphicsItem>
#include <QPainter>

class Rectangle : public QGraphicsItem
{
public:
    Rectangle(int width, int height, const QColor& color);

    void setColor(const QColor& color);
    void setSize(int width, int height);

    QRectF boundingRect() const override;

private:
    QRectF boundingRect_;
    QColor color_;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
};

inline Rectangle::Rectangle(int width, int height, const QColor& color)
    : QGraphicsItem()
    , boundingRect_(0, 0, width, height)
    , color_(color)
{

}

inline void Rectangle::setColor(const QColor& color)
{
    color_ = color;
}

inline void Rectangle::setSize(int width, int height)
{
    boundingRect_.setWidth(width);
    boundingRect_.setHeight(height);
}

inline QRectF Rectangle::boundingRect() const
{
    return boundingRect_;
}

inline void Rectangle::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setPen(Qt::NoPen);
    painter->setBrush(color_);
    painter->drawRect(boundingRect_);
}
