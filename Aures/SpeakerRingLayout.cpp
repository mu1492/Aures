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
SpeakerRingLayout.cpp

This file contains the sources for the speaker ring layout.
*/

#include "SpeakerRingLayout.h"

#include "MultiSourceHandler.h"
#include "Numeric.h"

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
SpeakerRingLayout::SpeakerRingLayout
    (
    QWidget*    aParent     //!< parent widget
    )
    : QDial( aParent )
{
    setMinimum( static_cast<int>( MultiSourceHandler::FIRST_SPEAKER_ANGLE_DEG_MIN ) );
    setMaximum( static_cast<int>( MultiSourceHandler::FIRST_SPEAKER_ANGLE_DEG_MAX ) );
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
void SpeakerRingLayout::paintEvent
    (
    QPaintEvent*    aEvent  //!< paint event
    )
{
    Q_UNUSED( aEvent );

    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing );
    const QPointF CENTER = rect().center();

    //*//////////
    // notches
    //*//////////
    const double NOTCH_RADIUS = qMin( width(), height() ) / 2.0 - 2;
    const double NOTCH_LENGTH = rect().width() / 12.0;
    QPen notchPenMain( QColor( Qt::red ), 7, Qt::SolidLine, Qt::RoundCap );
    QPen notchPenRest( QColor( Qt::red ), 4, Qt::SolidLine, Qt::RoundCap );
    const double SPAN_ANGLE_DEG = 360;
    const int NOTCHES_COUNT = SPAN_ANGLE_DEG / notchTarget();
    Numeric* numericInstance = Numeric::getInstance();

    for( int i = 0; i < NOTCHES_COUNT; i++ )
    {
        if( 0 == i )
        {
            painter.setPen( notchPenMain );
        }
        else
        {
            painter.setPen( notchPenRest );
        }

        const double ANGLE_DEG = 90 - value() + static_cast<double>( i ) * SPAN_ANGLE_DEG / NOTCHES_COUNT;
        const double ANGLE_RAD = numericInstance->deg2Rad( ANGLE_DEG );

        const QPointF outerPoint( CENTER.x() + cos( ANGLE_RAD ) * NOTCH_RADIUS,
                                  CENTER.y() - sin( ANGLE_RAD ) * NOTCH_RADIUS );

        const QPointF innerPoint( CENTER.x() + cos( ANGLE_RAD ) * ( NOTCH_RADIUS - NOTCH_LENGTH ),
                                  CENTER.y() - sin( ANGLE_RAD ) * ( NOTCH_RADIUS - NOTCH_LENGTH ) );

        painter.drawLine( outerPoint, innerPoint );
    }

    //*//////////
    // mic array
    //*//////////
    QFont font = painter.font();
    font.setPointSize( 9 );
    font.setBold( true );
    painter.setFont( font );
    const int LABEL_WIDTH = 18;
    const double SCALE_RATIO = 13.41 * rect().width();

    for( size_t i = 0; i < mMicArray.getArray().size(); i++ )
    {
        double xM = SCALE_RATIO * mMicArray.getArray().at( i ).getXyzLocation().x;
        double yM = SCALE_RATIO * mMicArray.getArray().at( i ).getXyzLocation().y;

        QRect circleRect( CENTER.x() + xM - LABEL_WIDTH / 2,
                          CENTER.y() - yM - LABEL_WIDTH / 2,
                          LABEL_WIDTH, LABEL_WIDTH );

        painter.setBrush( Qt::white );
        painter.setPen( Qt::NoPen );
        painter.drawEllipse( circleRect );

        QString micLabel = QString::number( mMicArray.getArray().at( i ).getLabel() );
        painter.setPen( Qt::black );
        painter.drawText( circleRect, Qt::AlignCenter, micLabel );
    }
}


//!************************************************************************
//! Update the mic array
//!
//! @returns nothing
//!************************************************************************
void SpeakerRingLayout::updateMicArray
    (
    MicArray    aMicArray   //!< mic array
    )
{
    mMicArray = aMicArray;
    update();
}
