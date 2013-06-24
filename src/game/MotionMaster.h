/*
 * This file is part of the BlizzLikeCore Project.
 * See CREDITS.md and LICENSE.md files for Copyright information.
 */

#ifndef BLIZZLIKE_MOTIONMASTER_H
#define BLIZZLIKE_MOTIONMASTER_H

#include "Common.h"
#include <vector>
#include "SharedDefines.h"

class Unit;
class MovementGenerator;

// Creature Entry ID used for waypoints show, visible only for GMs
#define VISUAL_WAYPOINT 1

// values 0 ... MAX_DB_MOTION_TYPE-1 used in DB
enum MovementGeneratorType
{
    IDLE_MOTION_TYPE      = 0,                              // IdleMovementGenerator.h
    RANDOM_MOTION_TYPE    = 1,                              // RandomMovementGenerator.h
    WAYPOINT_MOTION_TYPE  = 2,                              // WaypointMovementGenerator.h
    MAX_DB_MOTION_TYPE    = 3,                              // *** this and below motion types can't be set in DB.
    ANIMAL_RANDOM_MOTION_TYPE = MAX_DB_MOTION_TYPE,         // AnimalRandomMovementGenerator.h
    CONFUSED_MOTION_TYPE  = 4,                              // ConfusedMovementGenerator.h
    TARGETED_MOTION_TYPE  = 5,                              // TargetedMovementGenerator.h
    HOME_MOTION_TYPE      = 6,                              // HomeMovementGenerator.h
    FLIGHT_MOTION_TYPE    = 7,                              // WaypointMovementGenerator.h
    POINT_MOTION_TYPE     = 8,                              // PointMovementGenerator.h
    FLEEING_MOTION_TYPE   = 9,                              // FleeingMovementGenerator.h
    DISTRACT_MOTION_TYPE  = 10,                             // IdleMovementGenerator.h
    ASSISTANCE_MOTION_TYPE= 11,                             // PointMovementGenerator.h (first part of flee for assistance)
    ASSISTANCE_DISTRACT_MOTION_TYPE = 12,                   // IdleMovementGenerator.h (second part of flee for assistance)
    TIMED_FLEEING_MOTION_TYPE = 13,                         // FleeingMovementGenerator.h (alt.second part of flee for assistance)
    ROTATE_MOTION_TYPE    = 14,
    NULL_MOTION_TYPE      = 15,
};

enum MovementSlot
{
    MOTION_SLOT_IDLE,
    MOTION_SLOT_ACTIVE,
    MOTION_SLOT_CONTROLLED,
    MAX_MOTION_SLOT,
};

enum MMCleanFlag
{
    MMCF_NONE   = 0,
    MMCF_UPDATE = 1, // Clear or Expire called from update
    MMCF_RESET  = 2  // Flag if need top()->Reset()
};

enum RotateDirection
{
    ROTATE_DIRECTION_LEFT,
    ROTATE_DIRECTION_RIGHT,
};

// assume it is 25 yard per 0.6 second
#define SPEED_CHARGE    42.0f

class MotionMaster //: private std::stack<MovementGenerator *>
{
    private:
        //typedef std::stack<MovementGenerator *> Impl;
        typedef MovementGenerator* _Ty;
        _Ty Impl[MAX_MOTION_SLOT];
        bool needInit[MAX_MOTION_SLOT];
        typedef std::vector<_Ty> ExpireList;
        int i_top;

        void pop() { Impl[i_top] = NULL; --i_top; }
        void push(_Ty _Val) { ++i_top; Impl[i_top] = _Val; }

        bool needInitTop() const { return needInit[i_top]; }
        void InitTop();

    public:

        bool empty() const { return i_top < 0; }
        explicit MotionMaster(Unit* unit) : i_owner(unit), m_expList(NULL), m_cleanFlag(MMCF_NONE), i_top(-1)
        {
            for (uint8 i = 0; i < MAX_MOTION_SLOT; ++i)
            {
                Impl[i] = NULL;
                needInit[i] = true;
            }
        }
        ~MotionMaster();

        void Initialize();
        void InitDefault();

        int size() const { return i_top + 1; }
        _Ty top() const { return Impl[i_top]; }
        _Ty GetMotionSlot(int slot) const { return Impl[slot]; }

        void DirectDelete(_Ty curr);
        void DelayedDelete(_Ty curr);

        void UpdateMotion(uint32 diff);
        void Clear(bool reset = true)
        {
            if (m_cleanFlag & MMCF_UPDATE)
            {
                if (reset)
                    m_cleanFlag |= MMCF_RESET;
                else
                    m_cleanFlag &= ~MMCF_RESET;
                DelayedClean();
            }
            else
                DirectClean(reset);
        }
        void MovementExpired(bool reset = true)
        {
            if (m_cleanFlag & MMCF_UPDATE)
            {
                if (reset)
                    m_cleanFlag |= MMCF_RESET;
                else
                    m_cleanFlag &= ~MMCF_RESET;
                DelayedExpire();
            }
            else
                DirectExpire(reset);
        }

        void MoveIdle(MovementSlot slot = MOTION_SLOT_ACTIVE);
        void MoveTargetedHome();
        void MoveRandom(float spawndist = 0.0f);
        void MoveFollow(Unit* target, float dist, float angle, MovementSlot slot = MOTION_SLOT_ACTIVE);
        void MoveChase(Unit* target, float dist = 0.0f, float angle = 0.0f);
        void MoveConfused();
        void MoveFleeing(Unit* enemy, uint32 time = 0);
        void MovePoint(uint32 id, const Position &pos)
            { MovePoint(id, pos.m_positionX, pos.m_positionY, pos.m_positionZ); }
        void MovePoint(uint32 id, float x, float y, float z, bool usePathfinding = true);
        void MoveCharge(float x, float y, float z, float speed = SPEED_CHARGE, uint32 id = EVENT_CHARGE, bool usePathfinding = true);
        void MoveFall(float z, uint32 id = 0);
        void MoveJumpTo(float angle, float speedXY, float speedZ);
        void MoveJump(float x, float y, float z, float speedXY, float speedZ, bool usePathfinding = true);
        void MoveSeekAssistance(float x,float y,float z);
        void MoveSeekAssistanceDistract(uint32 timer);
        void MoveTaxiFlight(uint32 path, uint32 pathnode);
        void MoveDistract(uint32 time);
        void MovePath(uint32 path_id, bool repeatable);
        void MoveRotate(uint32 time, RotateDirection direction);

        // given destination unreachable? due to pathfinding or other
        virtual bool isReachable() const { return true; }

        MovementGeneratorType GetCurrentMovementGeneratorType() const;
        MovementGeneratorType GetMotionSlotType(int slot) const;

        void propagateSpeedChange();

        bool GetDestination(float &x, float &y, float &z);
    private:
        void Mutate(MovementGenerator *m, MovementSlot slot);                  // use Move* functions instead

        void DirectClean(bool reset);
        void DelayedClean();

        void DirectExpire(bool reset);
        void DelayedExpire();

        Unit       *i_owner;
        ExpireList *m_expList;
        uint8       m_cleanFlag;
};
#endif