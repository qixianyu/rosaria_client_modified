#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <signal.h>
#include <termios.h>
#include <stdio.h>
#include <boost/thread.hpp>
#define KEYCODE_R 0x43
#define KEYCODE_L 0x44
#define KEYCODE_U 0x41
#define KEYCODE_D 0x42
#define KEYCODE_Q 0x71
#define KEYCODE_SPACE 0x20

int kfd = 0;
struct termios cooked, raw;

class TeleopRosAria
{
  public:
    TeleopRosAria();
    ~TeleopRosAria()
    {
       tcsetattr(kfd, TCSANOW, &cooked);
    }
    void keyLoop();
    void publish_velocity_loop();
  private:
    ros::NodeHandle nh_;
    double linear_vel_max_, angular_vel_max_;
    ros::Publisher twist_pub_;
    geometry_msgs::Twist twist_;
    double linear_vel_step;
    double angular_vel_step;
    boost::thread* p_read_key_thread_;
    bool quit_requested;

};

TeleopRosAria::TeleopRosAria():
  linear_vel_max_(3.4),
  angular_vel_max_(1.2),
  linear_vel_step(0.1),
  angular_vel_step(0.02),
  quit_requested(false)

{

    twist_.angular.z = 0;//initialize
    twist_.linear.x = 0;
  //nh_.param("scale_angular", a_scale_, a_scale_);
 // nh_.param("scale_linear", l_scale_, l_scale_);
    twist_pub_ = nh_.advertise<geometry_msgs::Twist>("RosAria/cmd_vel", 1);
    p_read_key_thread_ = new boost::thread(&TeleopRosAria::keyLoop,this);
}
void quit(int sig)
{
  tcsetattr(kfd, TCSANOW, &cooked);
  ros::shutdown();
  exit(0);
}

void TeleopRosAria::keyLoop()
{
  char c;
  // bool dirty=false;
  // get the console in raw mode
  // get the next event from the keyboard
  while(1)
  {
    
    if(read(kfd, &c, 1) < 0)
      {
        perror("read():");
        exit(-1);
        ROS_INFO_STREAM("error");
      }

    switch(c)
      {
        case KEYCODE_L:

              if(twist_.angular.z <=angular_vel_max_)
              {
                   twist_.angular.z += angular_vel_step;
              }
              ROS_INFO_STREAM("KeyOp: angular velocity incremented [" << twist_.linear.x << "|" << twist_.angular.z<< "]");

          break;
        case KEYCODE_R:
            if (twist_.angular.z >=-angular_vel_max_)
            {
                twist_.angular.z -= angular_vel_step;
            }
            ROS_INFO_STREAM("KeyOp: angular velocity decremented [" << twist_.linear.x << "|" << twist_.angular.z<< "]");

          break;
        case KEYCODE_U:

         if (twist_.linear.x <= linear_vel_max_)
            {
              twist_.linear.x += linear_vel_step;
            }
          ROS_INFO_STREAM("KeyOp: linear velocity decremented [" << twist_.linear.x << "|" << twist_.angular.z<< "]");

          break;
        case KEYCODE_D:
        if (twist_.linear.x >= -linear_vel_max_)
           {
             twist_.linear.x -= linear_vel_step;
           }
         ROS_INFO_STREAM("KeyOp: linear velocity decremented [" << twist_.linear.x << "|" << twist_.angular.z<< "]");
          break;
        case KEYCODE_SPACE:
         ROS_DEBUG("STOP");
         twist_.linear.x=0;
         twist_.angular.z=0;

          break;
      case KEYCODE_Q:
        ROS_DEBUG("QUIT");
        ROS_INFO_STREAM("You quit the teleop successfully");
        quit_requested=true;
        return;
        break;
    }
  }

}
void TeleopRosAria::publish_velocity_loop()
{
    ros::Rate rh(10);//10 ms
    while(!quit_requested && ros::ok())
    {
        twist_pub_.publish(twist_);
        ros::spinOnce();
        rh.sleep();
    }
    twist_.linear.x = 0.0;
    twist_.angular.z = 0.0;
    twist_pub_.publish(twist_);
    p_read_key_thread_->join();
    delete p_read_key_thread_;
    p_read_key_thread_ = NULL;
}

int main(int argc, char** argv)
{
  ros::init(argc, argv, "teleop_RosAria");
  ros::NodeHandle nh;
  signal(SIGINT,quit);
  tcgetattr(kfd, &cooked);
  memcpy(&raw, &cooked, sizeof(struct termios));
  raw.c_lflag &=~ (ICANON | ECHO);
  // Setting a new line, then end of file
  raw.c_cc[VEOL] = 1;
  raw.c_cc[VEOF] = 2;
  tcsetattr(kfd, TCSANOW, &raw);
  puts("Reading from keyboard");
  puts("---------------------------");
  puts("Use arrow keys to move the robot.");
  puts("Press the space bar to stop the robot.");
  puts("Press q to stop the program");
  TeleopRosAria teleop_RosAria;

  teleop_RosAria.publish_velocity_loop();
  return(0);
}

