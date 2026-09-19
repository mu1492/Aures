///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 Mihai Ursu                                                 //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
// (at your option) any later version.                                           //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

/*
BeampatternLayout.cpp

This file contains the sources for the beampattern layout.
*/

#include "BeampatternLayout.h"

#include "Beamforming.h"
#include "Numeric.h"

#include <algorithm>
#include <cmath>

#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QString>


//!************************************************************************
//! Constructor
//!************************************************************************
BeampatternLayout::BeampatternLayout
    (
    QWidget*    aParent     //!< parent widget
    )
    : QDial( aParent )
    , mShowSpeakers( true )
    , mFirstSpeakerAngleDeg( 0 )
    , mNrOfSpeakers( 1 )
{
    setMinimum( -180 );
    setMaximum( +180 );
    setSingleStep( 1 );
    setPageStep( 1 );
    setTracking( true );
    setNotchesVisible( true );
    setWrapping( true );
    setInvertedAppearance( false );
    setInvertedControls( false );
}


//!************************************************************************
//! Handle paint events
//!
//! @returns nothing
//!************************************************************************
void BeampatternLayout::paintEvent
    (
    QPaintEvent*    aEvent  //!< paint event
    )
{
    Q_UNUSED( aEvent );

    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing );
    const QPointF CENTER = rect().center();
    painter.setBrush( Qt::NoBrush );

    //*//////////
    // DoA
    //*//////////
    const double DOA_RADIUS = qMin( width(), height() ) / 2.0 - 2;
    const double DOA_LENGTH = rect().width() / 12.0;
    QPen doaPen( QColor( Qt::green ), 7, Qt::SolidLine, Qt::RoundCap );
    painter.setPen( doaPen );

    Numeric* numericInstance = Numeric::getInstance();

    const double DOA_ANGLE_DEG = 90 - value();
    const double DOA_ANGLE_RAD = numericInstance->deg2Rad( DOA_ANGLE_DEG );

    const QPointF outerPoint( CENTER.x() + cos( DOA_ANGLE_RAD ) * DOA_RADIUS,
                              CENTER.y() - sin( DOA_ANGLE_RAD ) * DOA_RADIUS );

    const QPointF innerPoint( CENTER.x() + cos( DOA_ANGLE_RAD ) * ( DOA_RADIUS - DOA_LENGTH ),
                              CENTER.y() - sin( DOA_ANGLE_RAD ) * ( DOA_RADIUS - DOA_LENGTH ) );

    painter.drawLine( outerPoint, innerPoint );

    //*//////////
    // speaker directions
    //*//////////
    if( mShowSpeakers )
    {
        const double DIR_RADIUS = 0.75 * qMin( width(), height() ) / 2.0;
        QPen dirPen( QColor( Qt::darkBlue ), 0.25, Qt::DotLine );
        painter.setPen( dirPen );
        const double SPAN_ANGLE_DEG = 360;
        const int DIR_COUNT = SPAN_ANGLE_DEG / mNrOfSpeakers;

        for( int i = 0; i < DIR_COUNT; i++ )
        {
            const double ANGLE_DEG = 90 + mFirstSpeakerAngleDeg + static_cast<double>( i ) * SPAN_ANGLE_DEG / mNrOfSpeakers;
            const double ANGLE_RAD = numericInstance->deg2Rad( ANGLE_DEG );

            const QPointF outerPoint( CENTER.x() + cos( ANGLE_RAD ) * DIR_RADIUS,
                                      CENTER.y() - sin( ANGLE_RAD ) * DIR_RADIUS );

            painter.drawLine( outerPoint, CENTER );
        }
    }

    //*//////////
    // beampattern
    //*//////////    
    // polar grid
    const int D0DB = 0.78 * rect().width();
    QRect circle0dB( CENTER.x() - 0.5 * D0DB, CENTER.y() - 0.5 * D0DB, D0DB, D0DB );
    QPen pen0dB( QColor( Qt::white ), 1, Qt::SolidLine );
    painter.setPen( pen0dB );
    painter.drawEllipse( circle0dB );

    const double RATIO_3DB = pow( 10.0, -3.0 / 20.0 );
    const int D_3DB = D0DB * RATIO_3DB;
    QRect circle_3dB( CENTER.x() - 0.5 * D_3DB, CENTER.y() - 0.5 * D_3DB, D_3DB, D_3DB );
    QPen pen_3dB( QColor( Qt::gray ), 1, Qt::DashLine );
    painter.setPen( pen_3dB );
    painter.drawEllipse( circle_3dB );

    const double RATIO_6DB = pow( 10.0, -6.0 / 20.0 );
    const int D_6DB = D0DB * RATIO_6DB;
    QRect circle_6dB( CENTER.x() - 0.5 * D_6DB, CENTER.y() - 0.5 * D_6DB, D_6DB, D_6DB );
    QPen pen_6dB( QColor( Qt::darkGray ), 1, Qt::DashLine );
    painter.setPen( pen_6dB );
    painter.drawEllipse( circle_6dB );

    // polar trace
    QPen penTrace( QColor( Qt::green ), 1.5, Qt::SolidLine );
    painter.setPen( penTrace );

    const size_t LEN = mBeampatternMagDbVec.size();

    if( 360 == LEN )
    {
        const int PLOT_R = 0.5 * D0DB - 1;
        QPoint pOld( CENTER.x(),
                     CENTER.y() + PLOT_R * pow( 10.0, mBeampatternMagDbVec.at( 0 ) / 20.0 ) );
        QPoint pFirst = pOld;

        for( int i = 1; i < LEN; i++ )
        {
            const double ANGLE_TO_MIC3_RAD = numericInstance->deg2Rad( i - 90.0 );
            const double TRACE_R = PLOT_R * pow( 10.0, mBeampatternMagDbVec.at( i ) / 20.0 );

            QPoint pNew( CENTER.x() + cos( ANGLE_TO_MIC3_RAD ) * TRACE_R,
                         CENTER.y() - sin( ANGLE_TO_MIC3_RAD ) * TRACE_R );
            painter.drawLine( pOld, pNew );
            pOld = pNew;
        }

        painter.drawLine( pOld, pFirst );
    }
}


//!************************************************************************
//! Set the status if speaker directions are shown
//!
//! @returns: nothing
//!************************************************************************
void BeampatternLayout::setShowSpeakers
    (
    const bool      aEnabled,               //!< state
    const double    aFirstSpeakerAngleDeg,  //!< angle [deg]
    const uint8_t   aNrOfSpeakers           //!< number of speakers
    )
{
    mShowSpeakers = aEnabled;
    mFirstSpeakerAngleDeg = aFirstSpeakerAngleDeg;
    mNrOfSpeakers = aNrOfSpeakers;

    update();
}


//!************************************************************************
//! Update the beampattern plot
//! Vector is expected to have a length of 360.
//! ** trigonometric sense **
//! -> see BeampatternThread::run()
//!
//! |-------------------------------------------|
//! |           |     Angle     |     Angle     |
//! |  Vector   |  relative to  |  relative to  |
//! |  element  |  mic 5 [deg]  |  mic 3 [deg]  |
//! |           |               |               |
//! |     i     |    i-180      |     i-90      |
//! |-------------------------------------------|
//! |      0    |     -180      |      -90      |
//! |     90    |      -90      |        0      |
//! |    180    |        0      |      +90      |
//! |    270    |      +90      |     +180      |
//! |    359    |     +179      |     +269      |
//! |-------------------------------------------|
//!
//! @returns nothing
//!************************************************************************
void BeampatternLayout::updateBeampatternPlot
    (
    CxVector    aVector   //!< beampattern values
    )
{
    const size_t LEN = aVector.size();

    if( 360 == LEN )
    {
        Beamforming bf;
        mBeampatternMagDbVec.clear();

        for( size_t i = 0; i < LEN; i++ )
        {
            mBeampatternMagDbVec.push_back( bf.convertBeampatternCxToMagnitudeDb( aVector.at( i ) ) );
        }

        double maxVal = *std::max_element( mBeampatternMagDbVec.begin(), mBeampatternMagDbVec.end() );

        for( size_t i = 0; i < mBeampatternMagDbVec.size(); i++ )
        {
            mBeampatternMagDbVec.at( i ) -= maxVal;
        }

        update();
    }
}
