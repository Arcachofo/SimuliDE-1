/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "picicunit.h"
#include "datautils.h"
#include "mcupin.h"

PicIcUnit::PicIcUnit( eMcu* mcu, QString name )
         : McuIcUnit( mcu, name )
{
}
PicIcUnit::~PicIcUnit( ){}

void PicIcUnit::configure( uint8_t CCPxM ) // CCPxM0,CCPxM1,CCPxM2,CCPxM3
{
    m_enabled = true;
    m_fallingEdge = false;
    m_icPin->changeCallBack( this, true );

    switch( CCPxM ) {
        case 4: m_fallingEdge = true; break; // Falling Edge
        case 5:                       break; // Rising Edge
        case 6: m_prescaler = 4;      break; // Rising Edge, Presc = 4
        case 7: m_prescaler = 16;            // Rising Edge, Presc = 16
    }
}
