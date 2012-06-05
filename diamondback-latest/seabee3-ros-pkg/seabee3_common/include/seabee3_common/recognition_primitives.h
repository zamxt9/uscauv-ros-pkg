/***************************************************************************
 *  include/seabee3_common/recognition_primitives.h
 *  --------------------
 *
 *  Copyright (c) 2011, Edward T. Kaszubski ( ekaszubski@gmail.com )
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of seabee3-ros-pkg nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **************************************************************************/

#ifndef SEABEE3COMMON_RECOGNITIONPRIMITIVES_H_
#define SEABEE3COMMON_RECOGNITIONPRIMITIVES_H_

#include <map>
#include <quickdev/action_token.h>
#include <seabee3_common/colors.h>
#include <seabee3_common/motion_primitives.h>

namespace seabee
{

// =============================================================================================================================================
class Landmark
{
public:
    Pose pose_;

    enum LandmarkType
    {
        GATE,
        BUOY,
        PIPE,
        HEDGE,
        WINDOW,
        BIN,
        PINGER
    };

    LandmarkType type_;
    Color color_;
    std::string name_;

    template
    <
        class... __Args,
        typename std::enable_if<(!sizeof...(__Args) == 1 || !std::is_same<
            typename std::remove_reference<
                typename std::remove_const<
                    typename variadic::element<0, __Args...>::type
                >::type
            >::type,
            Landmark
        >::value), int>::type = 0
    >
    Landmark( __Args&&... args )
    {
        init( args... );
    }

    // ##[ Initialization ]#####################################################################################################################
    template<class... __Args>
    void init( std::string const & name, __Args&&... args )
    {
        name_ = name;
        init( args... );
    }

    template<class... __Args>
    void init( LandmarkType const & type, __Args&&... args )
    {
        type_ = type;
        init( args... );
    }

    template<class... __Args>
    void init( Color const & color, __Args&&... args )
    {
        color_ = color;
        init( args... );
    }

    void init() const {}
};

// =============================================================================================================================================
class Buoy : public Landmark
{
public:
    template<class... __Args>
    Buoy( __Args&&... args )
    :
        Landmark( Landmark::BUOY, args... )
    {
        //
    }
};

// =============================================================================================================================================
class Pipe : public Landmark
{
public:
    template<class... __Args>
    Pipe( __Args&&... args )
    :
        Landmark( Landmark::PIPE, Color::ORANGE, args... )
    {
        //
    }
};

// =============================================================================================================================================
// =============================================================================================================================================

    // #########################################################################################################################################
    //! Update the existing landmark filter in-place
    /*!
     * - Passing a narrowing filter item follwed by a widening filter item will remove the narrowing filter item
     * - Passing a widening filter item followed by a narrowing filter item will remove the widening filter item
     * - Passing two or more narrowing filter items will join the filter items
     * - Passing an inverted filter item will widen the search, ie { Buoy(), -Buoy( RED ) } will result in all buoys except for red ones
     * - Passing only an inverted filter item will function as if Landmark() was prepended to the list of arguments
     *
     * - Passing just a Landmark() will clear the filter and accept any kind of landmark
     * - Passing a Buoy() will accept any kind of buoy
     * - Passing a Buoy( seabee_common::colors::RED ) will only accept red buoys
     *
     * - Passing { Landmark(), Buoy(), Pipe(), Buoy( RED ) } will result in only pipes and red buoys
     * - Passing { Landmark(), -Buoy( RED ), -Buoy( GREEN ), Pipe() } will result in pipes and yellow buoys
     */
    template<class... __Args>
    void updateLandmarkFilter( Landmark const & landmark, __Args&&... landmarks );

    // #########################################################################################################################################
    //! Only look for landmarks that match the given filter
    /*!
     * - Calling this function will reset the filter, then apply the given changes
     *
     * - To accept all landmarks, pass: Landmark()
     * - To accept no landmarks, pass: -Landmark()
     */
    template<class... __Args>
    void setLandmarkFilter( Landmark const & landmark, __Args&&... landmarks );

    // #########################################################################################################################################
    //! Set (not update) the landmark filter to the specified value and return the resulting landmarks
    template<class... __Args>
    std::map<std::string, Landmark> getLandmarks( Landmark const & landmark, __Args&&... landmarks );

    //! Just get the resulting landmarks
    std::map<std::string, Landmark> getLandmarks();

} // seabee

#endif // SEABEE3COMMON_RECOGNITIONPRIMITIVES_H_
