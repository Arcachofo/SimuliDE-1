/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#ifndef USARTSR_H
#define USARTSR_H

#include "usartmodule.h"

class UartSr : public UartTR
{
    public:
        UartSr( UsartModule* usart, eMcu* mcu, QString name );
        ~UartSr();

        virtual void enable( uint8_t en ) override;
        virtual void runEvent() override;

        void processData( uint8_t data );
        void startTransmission();

    protected:

        IoPin* m_clkPin;
};

#endif
