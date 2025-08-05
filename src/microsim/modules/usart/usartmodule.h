/***************************************************************************
 *   Copyright (C) 2020 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#ifndef USARTMODULE_H
#define USARTMODULE_H

#include<QList>

#include "mcumodule.h"
#include "e-element.h"

#define mSTOPBITS m_usart->stopBits()
#define mDATABITS m_usart->dataBits()
#define mDATAMASK m_usart->dataMask()
#define mPARITY   m_usart->parity()

enum parity_t{
    parNONE=0,
    parEVEN=2,
    parODD=3,
};

enum rxError_t{
    frameError =1<<12,
    dataOverrun=1<<13,
    parityError=1<<14
};

class IoPin;
class UartTx;
class UartRx;
class UartSr;
class SerialMonitor;

class UsartModule
{
        friend class eMcu;
    public:
        UsartModule( eMcu* mcu, QString name );
        ~UsartModule();

        int baudRate() { return m_baudRate; }
        void setBaudRate( int br );

        int dataBits() { return m_dataBits; }
        void setDataBits( int b );

        uint8_t dataMask() { return m_dataMask; }

        int stopBits() { return m_stopBits; }
        void setStopBits( int sb ) { m_stopBits = sb; }

        parity_t parity() { return m_parity; }
        void setParity( int par ) { m_parity = (parity_t)par; }

        bool serialMon() { return m_serialMon; }

        virtual uint8_t getBit9Tx(){return 0;}
        virtual void setBit9Rx( uint8_t bit ){;}

        virtual void sendByte( uint8_t data );
        virtual void bufferEmpty(){;}
        virtual void frameSent( uint8_t data );
        virtual void readByte( uint8_t data ){;}
        virtual void byteReceived( uint8_t data );
        virtual void setRxFlags( uint16_t frame ){;}

        void openMonitor( QString id, int num=0, bool send=false );
        void setMonitorTittle( QString t );
        virtual void monitorClosed();


    protected:
        void setPeriod( uint64_t period );

        SerialMonitor* m_monitor;

        UartTx* m_sender;
        UartRx* m_receiver;
        UartSr* m_shiftReg;

        bool m_sync;
        bool m_serialMon;

        int m_baudRate;

        uint8_t m_mode;
        uint8_t m_stopBits;
        parity_t m_parity;

    private:
        uint8_t m_dataBits;
        uint8_t m_dataMask;
};

class Interrupt;

class UartTR : public McuModule, public eElement
{
        friend class McuCreator;

    public:
        UartTR( UsartModule* usart, eMcu* mcu, QString name );
        ~UartTR();

        enum state_t{
            usartSTOPPED=0,
            usartIDLE,
            usartTRANSMIT,
            usartTXEND,
            usartRECEIVE,
        };

        virtual void initialize() override;

        bool isEnabled() { return m_enabled; }
        virtual void enable( uint8_t e ){ m_enabled = e; }
        virtual uint8_t getData() { return  m_data; }

        virtual void configureA( uint8_t val ) override;

        void setPeriod( uint64_t period ) { m_period = period; }
        bool getParity( uint16_t data );

        state_t state() { return m_state; }

        void setPins( QList<IoPin*> pinList );
        IoPin* getPin() { return m_ioPin; }

        void raiseInt( uint8_t data=0 );

    protected:
        UsartModule* m_usart;
        IoPin* m_ioPin;
        QList<IoPin*> m_pinList;

        uint8_t m_buffer;
        uint8_t m_data;
        uint16_t m_frame;
        uint8_t m_framesize;
        uint8_t m_currentBit;
        uint8_t m_bit9;
        state_t m_state;

        bool m_enabled;

        uint64_t m_period; // Baudrate period
};

#endif
