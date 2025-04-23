/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "avricunit.h"
#include "datautils.h"
#include "e_mcu.h"
#include "mcupin.h"
#include "mcutimer.h"
#include "mcuinterrupts.h"

AvrIcUnit::AvrIcUnit( eMcu* mcu, QString name )
         : McuIcUnit( mcu, name )
{
    QString n = m_name.right(1);
    bool ok = false;
    n.toInt( &ok );
    if( !ok ) n = "";

    m_ICbits = getRegBits("ICES"+n+",ICNC"+n, mcu );
}
AvrIcUnit::~AvrIcUnit( ){}

void AvrIcUnit::configure( uint8_t val ) // ICES,ICNC
{
    uint8_t ICbits = getRegBitsVal( val, m_ICbits );
    m_fallingEdge = (ICbits & 1) == 0;

    /// TODO INCN
}
