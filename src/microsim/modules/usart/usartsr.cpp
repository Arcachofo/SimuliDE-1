/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "usartsr.h"
#include "simulator.h"
#include "iopin.h"

UartSr::UartSr( UsartModule* usart, eMcu* mcu, QString  name )
      : UartTR( usart, mcu, name )
{
    m_period = 0;
    m_framesize = 8;
    m_clkPin = nullptr;
}
UartSr::~UartSr( ){}

void UartSr::enable( uint8_t en )
{
    if( !m_clkPin ) m_clkPin = m_pinList.at(1);

    bool enabled = en > 0;
    if( enabled == m_enabled ) return;
    m_enabled = enabled;

    if( enabled ){
        m_state = usartIDLE;
        //m_ioPin->setOutState( true );
    }
    else m_state = usartSTOPPED;
}

void UartSr::runEvent()
{
    if( m_state == usartSTOPPED ) return;
    if( m_state == usartTRANSMIT )
    {
        m_clkPin->setOutState( true );

        if( ++m_currentBit == m_framesize ) m_state = usartTXEND; // Data transmission finished
        else                                m_clkPin->scheduleState( false, m_period/2 );

        m_ioPin->setOutState( m_frame & 1 );
        m_frame >>= 1;

        Simulator::self()->addEvent( m_period, this ); // Shedule next bit
    }
    else if( m_state == usartTXEND )
    {
        m_state = usartIDLE;
        m_ioPin->setOutState( true );
        m_usart->frameSent( m_data );
    }
}

void UartSr::processData( uint8_t data )
{
    m_buffer = data;
    if( m_enabled && m_state == usartIDLE ) startTransmission();
}

void UartSr::startTransmission() // Data loaded to ShiftReg
{
    m_usart->bufferEmpty();
    m_data = m_frame = m_buffer;
    m_currentBit = 0;
    m_state = usartTRANSMIT;

    m_clkPin->setOutState( true );
    m_ioPin->setOutState( true );

    if( m_period )
    {
        //sendBit(); // Shedule next bit
        Simulator::self()->addEvent( m_period, this ); // Shedule next bit
    }
}
