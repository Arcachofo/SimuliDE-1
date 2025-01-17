/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#ifndef CHIP_H
#define CHIP_H

#include "component.h"
#include "e-element.h"

class QDomElement;

class Chip : public Component, public eElement
{
    public:
        Chip( QString type, QString id, QString device="" );
        ~Chip();

        bool isBoard() { return m_isBoard; }
        
        bool logicSymbol() { return m_isLS; }
        virtual void setLogicSymbol( bool ls );

        virtual void setBackground( QString bck ) override;

        QString name() { return m_name; }
        virtual void setName( QString name );

        //subcType_t subcType() { return m_subcType; }

        QString subcTypeStr() { return m_subcType; }
        virtual void setSubcTypeStr( QString s ){ m_subcType = s; }

        int pkgWidth() { return m_width; }

        void setBackData( std::vector<std::vector<int>>* d ) { m_backData = d; }

        void setMargins( QString margins );
        QString getMargins() { return m_margins; }

        virtual void setflip() override;

        virtual void paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w ) override;

 static QString getDevice( QString id );

    protected:
        virtual Pin* addPin( QString id, QString type, QString label,
                             int pos, int xpos, int ypos, int angle, int length=8, int space=0 ){return NULL;}

        virtual Pin* updatePin( QString id, QString type, QString label,
                                int pos, int xpos, int ypos, int angle, int space=0  ){return NULL;}

        void addNewPin( QString id, QString type, QString label,
                        int pos, int xpos, int ypos, int angle, int length=8, int space=0 );

        virtual void initChip();
        virtual void initPackage( QDomElement root );

        virtual void findHelp() override;

        int m_width;
        int m_height;
        int m_topMargin;
        int m_bottomMargin;
        int m_rightMargin;
        int m_leftMargin;
        QString m_margins;
        
        bool m_isLS;
        bool m_initialized;
        bool m_isBoard;
        //subcType_t m_subcType;
        QString m_subcType;

        QColor m_lsColor;
        QColor m_icColor;

        QString m_name;
        QString m_device;
        QString m_pkgeFile;     // file containig package defs
        QString m_dataFile;

        QList<Pin*> m_unusedPins;

        std::vector<std::vector<int>>* m_backData;

        QGraphicsTextItem m_label;
};

#endif
