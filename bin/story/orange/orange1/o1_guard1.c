

#process "guard"

class auto_move;
class auto_att_fwd;

core_quad_A, 6539, 
  {object_move:auto_move, -1621},
  {object_pulse:auto_att_fwd, -395},
  {object_move:auto_move, 1425},
  {object_downlink, -361, 
    {component_prong, // component 1
      {object_pulse:auto_att_fwd, -1128},
      {object_move:auto_move, 894},
      {object_uplink, 0},
      {object_move:auto_move, 0},
    }
  },

#code


enum
{
// These are the channels that processes in this stage
//  use to communicate with each other.
// There can be up to 8
CHANNEL_HARVESTER, // used by harvesters
CHANNEL_TARGET, // used for TARGET_FOUND messages
CHANNEL_HELP, // used for UNDER_ATTACK messages

};

enum
{
// these are codes for the first value of a broadcast message.
// they tell the recipient what kind of messge it is.
//  (the recipient needs the same enum declaration)
MESSAGE_NEXT_WELL_PLEASE, // harvester asks main base where to go
MESSAGE_GO_TO_WELL, // base tells harvester where to go
MESSAGE_WELL_FOUND,
MESSAGE_WELL_FOUND_ACK, // base acknowledged well found message
MESSAGE_SEEK_WELLS, // base tells harvester to look for new wells
MESSAGE_TARGET_FOUND,
MESSAGE_UNDER_ATTACK,
MESSAGE_REQUEST_FOLLOWER
};



// Process AI modes (these reflect the capabilities of the process)
enum
{
  MODE_GUARD, // process is circling builder
  MODE_FOLLOW, // process is following harvester
  MODE_WANDER, // wandering randomly (should only happen if nothing else to do)
  MODE_ATTACK, // process is attacking target
  MODE_ATTACK_PRIORITY, // process is attacking priority target
  MODES
};


// Targetting information
// Targetting memory allows processes to track targets (enemy or friend)
// The following enums are used as indices in the process' targetting memory
enum
{
  TARGET_PARENT, // a newly built process starts with its builder as entry 0
  TARGET_LEADER, // leader this process has found  
  TARGET_FRONT,
  TARGET_MAIN
};

int angle_to_leader;

// Variable declaration and initialisation
//  (note that declaration and initialisation cannot be combined)
//  (also, variables retain their values between execution cycles)
int core_x, core_y; // location of core
core_x = get_core_x(); // location is updated each cycle
core_y = get_core_y();
int angle; // direction process is pointing
 // angles are in integer degrees from 0 to 8192, with 0 being right,
 // 2048 down, 4096 left and 6144 up.
angle = get_core_angle(); // angle is updated each cycle

int mode; // what is the process doing? (should be one of the MODE enums)
int saved_mode; // will return to this mode after attacking something
int attack_x, attack_y;
int attack_angle;
int angle_to_target;
int distance_from_target;
int using_main_attack;


int move_x, move_y; // destination
int target_x, target_y;

int scan_result; // used to hold the results of a scan of nearby processes

int initialised;
if (initialised == 0)
{
 initialised = 1;
 attack_mode(1);
 special_AI(0, 304);
 mode = MODE_GUARD;
 listen_channel(CHANNEL_HARVESTER);
 listen_channel(CHANNEL_TARGET);
 listen_channel(CHANNEL_HELP);

}


using_main_attack = 0;


// What the process does next depends on its current mode
switch(mode)
{


 case MODE_WANDER:
// only do this if nothing else to do.
  if (distance_from_xy(move_x, move_y) < 300)
  {
   gosub start_wandering;
   break;
  }
  auto_move.move_to(move_x, move_y);
  saved_mode = MODE_WANDER;
  gosub scan_for_target;
  gosub listen_for_requests;
  break;
  

 case MODE_GUARD:
// guards parent
  saved_mode = MODE_GUARD;
  if (process[TARGET_PARENT].visible() <= 0) // base destroyed
  {
   gosub start_wandering;
   break;
  }
  angle_to_leader = process[TARGET_PARENT].target_angle();
  angle_to_leader += 1024;
  move_x = process[TARGET_PARENT].get_core_x() - cos(angle_to_leader, 700);
  move_y = process[TARGET_PARENT].get_core_y() - sin(angle_to_leader, 700);
  auto_move.move_to(move_x, move_y);
  gosub scan_for_target;
  gosub listen_for_requests;
  break;

 case MODE_FOLLOW:
  saved_mode = MODE_FOLLOW;
  if (process[TARGET_LEADER].visible() <= 0) // base destroyed
  {
   gosub start_wandering;
   break;
  }
  angle_to_leader = process[TARGET_LEADER].target_angle();
  angle_to_leader += 1024;
  move_x = process[TARGET_LEADER].get_core_x() - cos(angle_to_leader, 700);
  move_y = process[TARGET_LEADER].get_core_y() - sin(angle_to_leader, 700);
  auto_move.move_to(move_x, move_y);
  gosub listen_for_requests;
  gosub scan_for_target;
  break;


 case MODE_ATTACK_PRIORITY:
 case MODE_ATTACK:
  if (!process[TARGET_MAIN].visible())
  {
   if (target_destroyed(TARGET_MAIN))
    special_AI(1, 2);
   if (distance_from_xy_more(target_x, target_y, 500))
    auto_move.move_to(target_x, target_y);
     else
      mode = saved_mode;
   break;
  }
  if (process[TARGET_MAIN].distance_less(1000))
  {
   auto_att_fwd.fire_at(TARGET_MAIN, random(process[TARGET_MAIN].get_components()));
   using_main_attack = 1;
  }
  if ((process[TARGET_MAIN].get_core_speed_x() != 0
    || process[TARGET_MAIN].get_core_speed_y() != 0)
   && process[TARGET_MAIN].get_interface_capacity() > 0)
  {
   attack_angle = atan2(process[TARGET_MAIN].get_core_speed_y(), process[TARGET_MAIN].get_core_speed_x())
        + 4096;
   angle_to_target = process[TARGET_MAIN].target_angle() + 4096;
   distance_from_target = 600;
   if (arc_length(attack_angle, angle_to_target) > 1024)   
   {
    distance_from_target += arc_length(attack_angle, angle_to_target); //  - 1024;
    if (angle_difference(attack_angle, angle_to_target) < 0)
     attack_angle = angle_to_target + 1024;
      else
       attack_angle = angle_to_target - 1024;
   }
   move_x = process[TARGET_MAIN].get_core_x() + cos(attack_angle, distance_from_target);
   move_y = process[TARGET_MAIN].get_core_y() + sin(attack_angle, distance_from_target);
   if (distance_from_xy_less(move_x, move_y, 100))
    auto_move.turn_to_target(TARGET_MAIN, 0);
     else
      auto_move.move_to(move_x, move_y);
  }
   else
   {
    auto_move.approach_target(TARGET_MAIN, 0, 500);
   }
// save last known position in case the target goes out of sight:
   target_x = process[TARGET_MAIN].get_core_x();
   target_y = process[TARGET_MAIN].get_core_y(); 
  break;
 

} // end of mode switch


if (!using_main_attack)
 auto_att_fwd.attack_scan(0, 400, TARGET_FRONT);

//charge_interface_max();

//restore_self();
//repair_self();


exit;


start_wandering:
 mode = MODE_WANDER;
 move_x = 800 + random(world_x() - 1600);
 move_y = 800 + random(world_y() - 1600);
 return;
 



scan_for_target:
  if (scan_single(0,0,TARGET_MAIN,0,0,100,0b1000)) // 0b1000 means only processes with allocator
  {
   broadcast_target(-1,
                    CHANNEL_TARGET,
                    1, // priority 1
                    TARGET_MAIN, // target attached to transmission
                    MESSAGE_TARGET_FOUND, 
                    process[TARGET_MAIN].get_core_x(),
                    process[TARGET_MAIN].get_core_y());
   mode = MODE_ATTACK_PRIORITY; 
   special_AI(1, 6);
   return;
  }
  if (scan_single(0,0,TARGET_MAIN,0,5,100,0)) // minimum 5 components
  {
   broadcast_target(20000,
                    CHANNEL_TARGET,
                    0, // priority 1
                    TARGET_MAIN, // target attached to transmission
                    MESSAGE_TARGET_FOUND, 
                    process[TARGET_MAIN].get_core_x(),
                    process[TARGET_MAIN].get_core_y());
   mode = MODE_ATTACK_PRIORITY; 
   special_AI(1, 5);
   return;
  }
  if (scan_for_threat(0, 0, TARGET_MAIN))
  {
   mode = MODE_ATTACK;
   broadcast_target(15000,
                    CHANNEL_TARGET,
                    0, // priority 1
                    TARGET_MAIN, // target attached to transmission
                    MESSAGE_TARGET_FOUND, 
                    process[TARGET_MAIN].get_core_x(),
                    process[TARGET_MAIN].get_core_y());
   special_AI(1, 5);
  }
  return;






listen_for_requests:
// listen to other procs requesting followers.
  if (next_message()) // returns 1 if message received since previous cycle, and starts reading first message
  {
   switch(read_message())
   {
    case MESSAGE_REQUEST_FOLLOWER:
     if (random(2)) // may or may not decide to follow.
     {
      mode = MODE_FOLLOW;
      get_message_source(TARGET_LEADER); // saves message sender into targetting memory     
     }
     break;
     
    case MESSAGE_TARGET_FOUND:
     mode = MODE_ATTACK;
     get_message_target(TARGET_MAIN);
// set target_x/y to the source of the message. These will be updated later anyway.
     target_x = get_message_x();
     target_y = get_message_y();
     break;
   }
  }
  return;
