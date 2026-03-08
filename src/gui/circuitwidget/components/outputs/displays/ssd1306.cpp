/***************************************************************************
 *   Copyright (C) 2019 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QPainter>

#include "ssd1306.h"
#include "itemlibrary.h"
#include "simulator.h"
#include "circuitview.h"
#include "circuitwidget.h"
#include "circuit.h"
#include "iopin.h"

#include "doubleprop.h"
#include "stringprop.h"
#include "boolprop.h"
#include "intprop.h"

#define tr(str) simulideTr("Ssd1306",str)

Component* Ssd1306::construct( QString type, QString id )
{ return new Ssd1306( type, id ); }

LibraryItem* Ssd1306::libraryItem()
{
    return new LibraryItem(
        "SSD1306",
        "Displays",
        "ssd1306.png",
        "Ssd1306",
        Ssd1306::construct );
}

Ssd1306::Ssd1306( QString type, QString id )
       : Component( type, id )
       , TwiModule( id )
       //, m_pinRst( 270, QPoint(-32, 48), id+"-PinRst" , 0, this )
       //, m_pinDC ( 270, QPoint(-24, 48), id+"-PinDC"  , 0, this )
       //, m_pinCS ( 270, QPoint(-16, 48), id+"-PinCS"  , 0, this )
{
    m_graphical = true;

    m_rotate = true;

    m_address = m_cCode = 0b00111100; // 0x3A - 60

    m_pin.resize( 2 );
    m_clkPin = new IoPin( 270, QPoint(-48, 48), id+"-PinSck" , 0, this, openCo );
    m_clkPin->setLabelText( "SCL" );
    m_pin[0] = m_clkPin;
    TwiModule::setSclPin( m_clkPin );

    m_pinSda = new IoPin( 270, QPoint(-40, 48), id+"-PinSda" , 0, this, openCo );
    m_pin[1] = m_pinSda;
    m_pinSda->setLabelText( "SDA" );
    TwiModule::setSdaPin( m_pinSda );

    //m_pinRst.setLabelText( "Res" );
    //m_pinDC.setLabelText(  "DC" );
    //m_pinCS.setLabelText(  "CS" );

    setSize( 128,64 );

    setColorStr("White");
    
    Simulator::self()->addToUpdateList( this );
    
    setLabelPos(-32,-60, 0);
    setShowId( true );
    
    Ssd1306::initialize();

    addPropGroup( { tr("Main"), {
        new StrProp <Ssd1306>("Color",tr("Color"), "White,Blue,Yellow;"+tr("White")+","+tr("Blue")+","+tr("Yellow")
                             ,this, &Ssd1306::colorStr, &Ssd1306::setColorStr,0,"enum" ),

        new IntProp <Ssd1306>("Width", tr("Width"), "_px"
                             , this, &Ssd1306::width, &Ssd1306::setWidth, propNoCopy,"uint" ),

        new IntProp <Ssd1306>("Height", tr("Height"), "_px"
                             ,this,&Ssd1306::height, &Ssd1306::setHeight, propNoCopy,"uint" ),

        new BoolProp<Ssd1306>("Rotate", tr("Rotate"), ""
                             , this, &Ssd1306::imgRotated, &Ssd1306::setImgRotated ),
    }, 0} );

    addPropGroup( { tr("I2C"), {
        new IntProp <Ssd1306>("Control_Code", tr("I2C Address"), ""
                             , this, &Ssd1306::cCode, &Ssd1306::setCcode,0,"uint" ),

        new DoubProp<Ssd1306>("Frequency",tr("I2C Frequency"), "_kHz"
                             , this, &Ssd1306::freqKHz, &Ssd1306::setFreqKHz ),
    }, 0} );
}
Ssd1306::~Ssd1306(){}

void Ssd1306::initialize()
{
    TwiModule::initialize();

    clearDDRAM();
    reset() ;
    Ssd1306::updateStep();
}

void Ssd1306::stamp()
{
    setMode( TWI_SLAVE );
}

void Ssd1306::updateStep()
{
    update();
    if( !m_scrollSingle && !m_scroll ) return;
    if( Simulator::self()->isPaused() ) return;

    if( m_scrollSingle ){
        m_scrollSingle = false;
    }else{
        m_scrollCount++;
        if( m_scrollCount < m_scrollStep ) return;
        m_scrollCount = 0;
    }

    int maxX = m_width-1;
    bool scrollRight = false;
    if( m_scrollV ) scrollRight = m_scrollDir == 1;
    else            scrollRight = m_scrollDir == 2;

    for( int row=m_scrollStartY; row<=m_scrollEndY; row++ )
    {
        int dy = row;

        if( scrollRight )
        {
            uint8_t end = m_DDRAM[maxX][dy];
            for( int col=maxX; col>0; --col ) m_DDRAM[col][dy] = m_DDRAM[col-1][dy];
            m_DDRAM[0][dy] = end;
        }else{
            uint8_t start = m_DDRAM[0][dy];
            for( int col=0; col<maxX; ++col ) m_DDRAM[col][dy] = m_DDRAM[col+1][dy];
            m_DDRAM[maxX][dy] = start;
        }
    }
    if( !m_scrollV ) return;

    for( int col=0; col<maxX; ++col )
    {
        uint64_t ramCol = 0;

        for( int row=m_scrollEndY; row>=m_scrollStartY; row-- )
        {
            ramCol <<= 8;
            ramCol |= m_DDRAM[col][row];
        }
        if( ramCol == 0 ) continue;

        uint8_t nBits = (m_scrollEndY-m_scrollStartY+1)*8;
        uint64_t mask = (1ULL << m_vScrollOffset) - 1;

        uint64_t upper = (ramCol & mask) << (nBits-m_vScrollOffset);
        uint64_t lower = ramCol >> m_vScrollOffset ;
        ramCol = upper | lower;

        for( int row=m_scrollStartY; row<=m_scrollEndY; row++ )
        {
            m_DDRAM[col][row] = ramCol & 0xFF;
            ramCol >>= 8;
        }
    }
}

void Ssd1306::startWrite()
{
    m_start = 1;
}

void Ssd1306::readByte()
{
    TwiModule::readByte();

    if( m_start )  // Read Control byte
    {
        m_start = 0;
        if( (m_rxReg & 0b00111111) != 0 ){
            qDebug() << "OledController::readByte Control Byte Error";
            //return;
        }
        m_Co    = m_rxReg & 0b10000000;
        m_data  = m_rxReg & 0b01000000;
    }
    else if( m_data )      writeData();
    else if( m_readBytes ) parameter();
    else                   proccessCommand();

    if( !m_readBytes ) m_start = m_Co; // If Co bit then next byte should be Control Byte
}

void Ssd1306::writeData()
{
    m_DDRAM[m_addrX][m_addrY] = m_rxReg;
    if( m_addrMode & VERT_ADDR_MODE )
    {
        m_addrY++;
        if( m_addrY > m_endY ){
            m_addrY = m_startY;
            if( m_addrMode != VERT_ADDR_MODE ) return;
            m_addrX++;
            if( m_addrX > m_endX ) m_addrX = m_startX;
        }
    }else{
        m_addrX++;
        if( m_addrX > m_endX ){
            m_addrX = m_startX;
            if( m_addrMode != HORI_ADDR_MODE ) return;
            m_addrY++;
            if( m_addrY > m_endY ) m_addrY = m_startY;
        }
    }
}

void Ssd1306::proccessCommand()
{
    m_lastCommand = m_rxReg;
    m_readIndex = 0;
    m_readBytes = 0;

    if( m_rxReg < 0x20 )
    {
        if( m_addrMode != PAGE_ADDR_MODE ) return;

        if( m_rxReg < 0x10 )                              // Lower Colum Start Address for Page Addresing mode
            m_addrX = (m_addrX & 0xF0) | (m_rxReg & 0x0F);
        else                                              // Higher Colum Start Address for Page Addresing mode
            m_addrX = (m_addrX & 0x0F) | ((m_rxReg & 0x0F) << 4);

        if( m_addrX >= m_maxWidth) m_addrX -= m_maxWidth;
    }
    else if( m_rxReg>=0x40 && m_rxReg<=0x7F )            // Display Start Line
    {
        m_ramOffset = m_rxReg & m_lineMask;
    }
    else if( m_rxReg>=0xB0 && m_rxReg<=0xB7 )            // Page Start Address for Page Addresing mode
    {
        if( m_addrMode == PAGE_ADDR_MODE ) m_addrY = m_rxReg & m_rowMask;
    }
    else{
        switch( m_rxReg )
        {
        case 0x20: m_readBytes = 1;    break; // Memory Addressing Mode
        case 0x21: m_readBytes = 2;    break; // Column Address (Start-End)
        case 0x22: m_readBytes = 2;    break; // Page Address (Start-End)
        case 0x23: m_readBytes = 1;    break; // Fade Out / Blinking Mode
        case 0x26:                            // Continuous Horizontal Right Scroll Setup
        case 0x27: m_readBytes = 6;    break; // Continuous Horizontal Left Scroll Setup
        case 0x29:                            // Continuous Vertical and Horizontal Right Scroll Setup
        case 0x2A: m_readBytes = 5;    break; // Continuous Vertical and Horizontal Left Scroll Setup
        case 0x2C:                            // One Column Horizontal Right Scroll Setup
        case 0x2D: m_readBytes = 6;    break; // One Column Horizontal Left Scroll Setup
        case 0x2E: m_scroll = false;   break; // Deactivate scroll
        case 0x2F: m_scroll = true;           // Activate scroll
            m_scrollCount = 0;
            break;
        case 0x81: m_readBytes = 1;    break; // Contrast Control
        case 0x8D: m_readBytes = 1;    break; // Charge Pump
        case 0xA0: m_remap = false;    break; // Segment Re-map OFF
        case 0xA1: m_remap = true;     break; // Segment Re-map ON
        case 0xA3: m_readBytes = 2;    break; // Vertical Scroll Area
        case 0xA4: m_dispFull = false; break; // Entire Display Off
        case 0xA5: m_dispFull = true;  break; // Entire Display ON
        case 0xA6: m_dispInv  = false; break; // Inverse Display OFF
        case 0xA7: m_dispInv  = true;  break; // Inverse Display ON
        case 0xA8: m_readBytes = 1;    break; // Multiplex Ratio
        case 0xAE: reset();            break; // Display OFF
        case 0xAF: m_dispOn = true;    break; // Display ON
        case 0xC0: m_scanInv = false;  break; // COM Output Scan Inverted OFF
        case 0xC8: m_scanInv = true;   break; // COM Output Scan Inverted ON
        case 0xD3: m_readBytes = 1;    break; // Display Offset
        case 0xD5: m_readBytes = 1;    break; // Display Clock Divide Ratio/Oscillator Frequency
        case 0xD6: m_readBytes = 1;    break; // Zoom in Mode
        case 0xD9: m_readBytes = 1;    break; // Precharge
        case 0xDA: m_readBytes = 1;    break; // COM Pins Hardware Configuration
        case 0xDB: m_readBytes = 1;    break; // VCOM DETECT
        }
    }
}

void Ssd1306::parameter()
{
    m_readIndex++;
    if( m_readIndex > m_readBytes ) return;
    if( m_readIndex == m_readBytes ) m_readBytes = 0;

    switch( m_lastCommand )
    {
    case 0x20: m_addrMode = m_rxReg & 3; break; // Memory Addressing Mode
    case 0x21:{                                 // Set Column Address (Start-End)
        if( m_addrMode == PAGE_ADDR_MODE ) return;
        if( m_readIndex == 1 ) m_addrX=m_startX = m_rxReg & 0x7F; // 0b01111111
        else                   m_endX   = m_rxReg & 0x7F; // 0b01111111
    }break;
    case 0x22:{                                 // 22 34 Set Page Address (Start-End)
        if( m_addrMode == PAGE_ADDR_MODE ) return;
        if( m_readIndex == 1 ) m_addrY=m_startY = m_rxReg & m_rowMask; // 0b00000111
        else                   m_endY   = m_rxReg & m_rowMask; // 0b00000111
    }break;
    case 0x26:                                       // Horizontal Right Scroll Setup
    case 0x27:                                       // Horizontal Left Scroll Setup
    case 0x29:                                       // Vertical and Horizontal Right Scroll Setup
    case 0x2A: configScroll( m_lastCommand ); break; // Vertical and Horizontal Left Scroll Setup
    case 0x2C:                                       // One Column Horizontal Right Scroll Setup
    case 0x2D: m_scrollSingle = true;                // One Column Horizontal Left Scroll Setup
        configScroll( m_lastCommand-6 );             // Same than 0x26/27
        break;
    case  0xA3:{                                     // Vertical Scroll Area
        switch( m_readIndex ) {
        case 1: m_scrollTop  = m_rxReg & m_lineMask; break;
        case 2: m_scrollRows = m_rxReg & 0x7F; break;
        }
    }break;
    case  0xA8: // Multiplex Ratio
    {
        uint8_t muxRatio = m_rxReg & m_lineMask;  // 0b00111111
        if( muxRatio > 14 ) m_mr = muxRatio;
    }break;
    case 0xD3: m_dispOffset = m_rxReg & m_lineMask; break; // Display Offset Set vertical shift by COM from 0d~63d
    }
}

void Ssd1306::configScroll( uint8_t command )
{
    m_scrollV   = command > 0x27;
    m_scrollDir = command & 0b11;

    switch( m_readIndex ) {
    case 1:                                       break;
    case 2: m_scrollStartY = m_rxReg & m_rowMask; break; // Define start page address
    case 3:{                                             // Scroll step time in terms of frame frequency
        switch( m_rxReg & m_rowMask ){
        case 0: m_scrollStep = 5;
        case 1: m_scrollStep = 64;
        case 2: m_scrollStep = 128;
        case 3: m_scrollStep = 256;
        case 4: m_scrollStep = 3;
        case 5: m_scrollStep = 4;
        case 6: m_scrollStep = 25;
        case 7: m_scrollStep = 2;
        }
    }break;
    case 4: m_scrollEndY    = m_rxReg & m_rowMask; break; // Define end page address
    case 5: m_vScrollOffset = m_rxReg & m_lineMask; break; // Vertical scrolling offset
    case 6: break;
    }
}

void Ssd1306::clearDDRAM() 
{
    for( int col=0; col<m_width; col++ )
        for( int row=0; row<m_rows; row++ )
            m_DDRAM[col][row] = 0;
}

void Ssd1306::reset()
{
    //m_cdr  = 1;
    m_mr   = 63;
    //m_fosc = 370000;
    //m_frm  = m_fosc/(m_cdr*54*m_mr);

    m_addrX  = 0;
    m_addrY  = 0;
    m_startX = 0;
    m_endX   = m_width-1;
    m_startY = 0;
    m_endY   = m_rows-1;

    m_scroll   = false;
    m_scrollV  = false;
    m_scrollDir = false;
    m_scrollStartY = 0;
    m_scrollEndY   = 7;
    m_scrollStep   = 5;
    m_vScrollOffset = 0;
    m_scrollSingle = false;

    m_ramOffset = 0;
    m_readBytes = 0;

    m_dispOn   = false;
    m_dispFull = false;
    m_dispInv  = false;
    m_scanInv  = false;
    m_remap    = false;

    m_addrMode = PAGE_ADDR_MODE;
}

void Ssd1306::setColorStr( QString color )
{
    m_dColor = color;

    if( color == "White"  ) m_foreground = QColor(245, 245, 245);
    if( color == "Blue"   ) m_foreground = QColor(200, 200, 255);
    if( color == "Yellow" ) m_foreground = QColor(245, 245, 100);

    if( m_showVal && (m_showProperty == "Color") )
        setValLabelText( color );
}

void Ssd1306::setWidth( int w )
{
    if     ( w > m_maxWidth ) w = m_maxWidth;
    else if( w < 32         ) w = 32;
    if( m_width == w ) return;
    m_width = w;
    updateSize();
}

void Ssd1306::setHeight( int h )
{
    if( h > m_height ) h += 8;
    if     ( h > m_maxHeight ) h = m_maxHeight;
    else if( h < 16          ) h = 16;

    h = (h/8)*8;
    if( m_height == h ) return;

    m_rows = h/8;
    m_lineMask = (h > 64) ? 0x7F : 0x3F;
    m_rowMask  = (h > 64) ? 0x0F : 0x07;
    m_height = h;
    updateSize();
}

void Ssd1306::setSize( int w, int h )
{
    if( Simulator::self()->isRunning() ) CircuitWidget::self()->powerCircOff();

    m_maxWidth = w;
    m_maxHeight = h;
    setWidth( w );
    setHeight( h );
    //updateSize();
    m_DDRAM.resize( m_width, std::vector<uint8_t>(m_rows, 0) );
}


void Ssd1306::updateSize()
{
    m_area = QRectF( -70, -m_height/2-16, m_width+12, m_height+24 );
    m_clkPin->setPos( QPoint(-48, m_height/2+16) );
    m_clkPin->isMoved();
    m_pinSda->setPos( QPoint(-40, m_height/2+16) );
    m_pinSda->isMoved();
    Circuit::self()->update();
}

void Ssd1306::paint( QPainter* p, const QStyleOptionGraphicsItem*, QWidget* )
{
    QPen pen( Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin );
    p->setPen( pen );

    p->setBrush( QColor( 50, 70, 100 ) );
    p->drawRoundedRect( m_area, 2, 2 );

    if( m_dispFull ) p->fillRect(-64,-m_height/2-10, m_width, m_height, m_foreground );
    else{
        QImage img( m_width*3, m_height*3, QImage::Format_RGB32 );
        QPainter painter;
        painter.begin( &img );
        painter.fillRect( 0, 0, m_width*3, m_height*3, Qt::black );

        if( m_dispOn ){
            for( int col=0; col<m_width; col++ ){
                for( int row=0; row<m_rows; row++ )
                {
                    int ramY = row*8;
                    if( m_ramOffset ){
                        ramY += m_ramOffset;
                        if( ramY >= m_height ) ramY -= m_height;
                    }
                    if( ramY > m_mr ) continue;

                    uint8_t rowByte = ramY/8;
                    uint8_t byte0 = m_DDRAM[col][rowByte];
                    if( m_dispInv ) byte0 = ~byte0;          // Display Inverted


                    uint8_t startBit = ramY%8;
                    uint8_t byte1 = 0;
                    if( startBit ){                          // bits spread 2 bytes
                        rowByte++;
                        byte1 = m_DDRAM[col][rowByte];
                        if( m_dispInv ) byte1 = ~byte1;      // Display Inverted
                    }
                    int dy = row*8;
                    if( m_dispOffset ){
                        dy += m_dispOffset;
                        if( dy >= m_height ) dy -= m_height;
                    }

                    for( int bit=startBit; bit<startBit+8; bit++ )
                    {
                        uint8_t pixel;
                        if( bit < 8 ) pixel = byte0 & 1<<bit;
                        else          pixel = byte1 & 1<<(bit-startBit);

                        if( pixel ){
                            int screenY = m_scanInv ? m_height-1-dy : dy;
                            int screenX = m_remap   ? m_width-1-col : col;
                            if( m_rotate ){
                                screenY = m_height-1-screenY;
                                screenX = m_width-1-screenX;
                            }
                            painter.fillRect( screenX*3, screenY*3, 3, 3, m_foreground );
                        }
                        dy++;
                        if( dy >= m_height ) dy -= m_height;
                    }
                }
            }
        }
        painter.end();
        p->drawImage( QRectF(-64,-m_height/2-10, m_width, m_height), img );
    }
    Component::paintSelected( p );
}
