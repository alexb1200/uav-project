#include <iostream>
#include <mavsdk.h>

#include <pybind11/pybind11.h>
#include <pybind11/eval.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>

#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <mavsdk/plugins/follow_me/follow_me.h>

#include <cstdint>
#include <atomic>
#include <thread>
#include <chrono>

#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

namespace py = pybind11;
using namespace py::literals;


using namespace mavsdk;
using namespace std::this_thread;
using namespace std::chrono;

static void takeoff_and_land(System& system);
static void moverand(System& system, pybind11::object& uav);
std::pair<double,double> initializeUAVs(System& system);
void getUavGoals(pybind11::object & uav);

#define ERROR_CONSOLE_TEXT "\033[31m" // Turn text on console red
#define TELEMETRY_CONSOLE_TEXT "\033[34m" // Turn text on console blue
#define NORMAL_CONSOLE_TEXT "\033[0m" // Restore normal console colour
PYBIND11_MAKE_OPAQUE(std::vector<int>);


//found it on stack overflow  https://stackoverflow.com/questions/26516683/reusing-thread-in-loop-c
class ThreadPool
{
    public:

    ThreadPool (int threads) : shutdown_ (false)
    {
        // Create the specified number of threads
        threads_.reserve (threads);
        for (int i = 0; i < threads; ++i)
            threads_.emplace_back (std::bind (&ThreadPool::threadEntry, this, i));
    }

    ~ThreadPool ()
    {
        {
            // Unblock any threads and tell them to stop
            std::unique_lock <std::mutex> l (lock_);

            shutdown_ = true;
            condVar_.notify_all();
        }

        // Wait for all threads to stop
        std::cerr << "Joining threads" << std::endl;
        for (auto& thread : threads_)
            thread.join();
    }

    void doJob (std::function <void (void)> func)
    {
        // Place a job on the queu and unblock a thread
        std::unique_lock <std::mutex> l (lock_);

        jobs_.emplace (std::move (func));
        condVar_.notify_one();
    }

    protected:

    void threadEntry (int i)
    {
        std::function <void (void)> job;

        while (1)
        {
            {
                std::unique_lock <std::mutex> l (lock_);

                while (! shutdown_ && jobs_.empty())
                    condVar_.wait (l);

                if (jobs_.empty ())
                {
                    // No jobs to do and we are shutting down
                    std::cerr << "Thread " << i << " terminates" << std::endl;
                    return;
                 }

                std::cerr << "Thread " << i << " does a job" << std::endl;
                job = std::move (jobs_.front ());
                jobs_.pop();
            }

            // Do the job without holding any locks
            job ();
        }

    }

    std::mutex lock_;
    std::condition_variable condVar_;
    bool shutdown_;
    std::queue <std::function <void (void)>> jobs_;
    std::vector <std::thread> threads_;
};



int main(int argc, char** argv) {
    std::cout << "Hello, world!\n";
     py::scoped_interpreter guard{};

        // Disable build of __pycache__ folders
        py::exec(R"(
            import sys
            import random
            import os
            import mdp
            print(sys.builtin_module_names)
            sys.dont_write_bytecode = True
        )");

        //srand(time(NULL));
    if (argc == 1) {
        std::cerr << ERROR_CONSOLE_TEXT << "Please specify connection" << NORMAL_CONSOLE_TEXT
                  << std::endl;
        return 1;
    }

    Mavsdk dc;
    
    int total_udp_ports = argc - 1;

    // the loop below adds the number of ports the sdk monitors.
    for (int i = 1; i < argc; ++i) {
        ConnectionResult connection_result = dc.add_any_connection(argv[i]);
        if (connection_result != ConnectionResult::Success) {
            std::cerr << ERROR_CONSOLE_TEXT << "Connection error: " << connection_result
                      << NORMAL_CONSOLE_TEXT << std::endl;
            return 1;
        }
    }

    std::atomic<signed> num_systems_discovered{0};

    std::cout << "Waiting to discover system..." << std::endl;
    dc.register_on_discover([&num_systems_discovered](uint64_t uuid) {
        std::cout << "Discovered system with UUID: " << uuid << std::endl;
        ++num_systems_discovered;
    });

    // We usually receive heartbeats at 1Hz, therefore we should find a system after around 2
    // seconds.
    sleep_for(seconds(2));

    if (num_systems_discovered != total_udp_ports) {
        std::cerr << ERROR_CONSOLE_TEXT << "Not all systems found, exiting." << NORMAL_CONSOLE_TEXT
                  << std::endl;
        return 1;
    }

    std::vector<std::thread> threads;
    std::vector<pybind11::object> pythonAgents;
   
    
    
    //make the python class instances here, so they can interact. 

    try{
             
        

                // get current location, initialize goal postions to give to python later etc.

                System & s= dc.system(dc.system_uuids().at(0) );
                std::pair<double,double> coords= initializeUAVs(s);

                double latGoal;
                double longiGoal;
                std::vector<std::pair<double,double>> xAndy;
                for(int i =0; i < 51; i++)
                {
                    latGoal   =  coords.first+(double)(rand()%1000-500)/10000.0f;
                    longiGoal =  coords.second+(double)(rand()%1000-500)/10000.0f;
                    xAndy.push_back(std::make_pair(latGoal,longiGoal));
                }
                // Initialize python
                Py_OptimizeFlag = 1;
                Py_SetProgramName(L"PythonEmbeddedExample");
                std::cout << "Importing module...   " << std::endl;
                auto uavFile = py::module::import("uav");

                std::cout << "Initializing class...   " << std::endl;
                const auto myExampleClass = uavFile.attr("uav");
                auto seller = myExampleClass(coords.first,coords.second,xAndy);
                //testing map
                getUavGoals(std::ref(seller));

                for(int i =0; i < total_udp_ports; i++){
                auto myExampleInstance = myExampleClass(coords.first,coords.second);
                pythonAgents.push_back(myExampleInstance);
                }
                const auto bidEnviroment = uavFile.attr("biddingEnviroment");
                auto runBid = bidEnviroment(pythonAgents,seller);
                std::cout<<"ran bid "<<std::endl;

                
    }
    catch(std::exception &e) {std::cerr << "Something went wrong: " << e.what() << std::endl;
               return EXIT_FAILURE;}

    std::vector<std::thread> threads1;
    std::cout<<"first for\n";
    int i=0;

    ThreadPool p (2);
    while(true){
    for (auto uuid : dc.system_uuids()) {

        System& system = dc.system(uuid);
        
        p.doJob(std::bind (& moverand, std::ref(system), std::ref(pythonAgents.at(i))) );
        
        i++;
    }

    i=0;

    }


        
    
//////////END MAVLINK

    return EXIT_SUCCESS;

}





std::pair<double,double> initializeUAVs(System& system)
{
    auto telemetry = std::make_shared<Telemetry>(system);
    auto action = std::make_shared<Action>(system);
    auto fm = std::make_shared<FollowMe>(system);
   

    // We want to listen to the altitude of the drone at 1 Hz.
    const Telemetry::Result set_rate_result = telemetry->set_rate_position(1.0);

    if (set_rate_result != Telemetry::Result::Success) {
        std::cerr << ERROR_CONSOLE_TEXT << "Setting rate failed:  " << set_rate_result
                  << NORMAL_CONSOLE_TEXT << std::endl;

    }

    // Set up callback to monitor altitude while the vehicle is in flight
   /*  telemetry->subscribe_position([](Telemetry::Position position) {
        std::cout << TELEMETRY_CONSOLE_TEXT // set to blue
                  << "Altitude: " << position.relative_altitude_m << " m"
                  << NORMAL_CONSOLE_TEXT // set to default color again
                  << std::endl;
    });
 */
    // Check if vehicle is ready to arm
    while (telemetry->health_all_ok() != true) {
        std::cout << "Vehicle is getting ready to arm" << std::endl;
        sleep_for(seconds(1));
    }



   

    double lat=telemetry->position().latitude_deg;
    double longi=telemetry->position().longitude_deg;
    return std::make_pair(lat,longi);
}

void getUavGoals(pybind11::object & uav)
{
    try { 
                auto posY = uav.attr("getListOfGoalsY")();
                auto posX = uav.attr("getListOfGoalsX")();

                auto logger = py::module::import("logger");

                auto plot = logger.attr("visual")(posX,posY);

               


            } catch (std::exception& e) {
                std::cerr << "Something went wrong: " << e.what() << std::endl;
               return ;
            }

}



void moverand(System& system, pybind11::object & uav) //pass in a python uav reference 
{
    
    auto telemetry = std::make_shared<Telemetry>(system);
    auto action = std::make_shared<Action>(system);
    auto fm = std::make_shared<FollowMe>(system);
   

    // We want to listen to the altitude of the drone at 1 Hz.
    const Telemetry::Result set_rate_result = telemetry->set_rate_position(1.0);

    if (set_rate_result != Telemetry::Result::Success) {
        std::cerr << ERROR_CONSOLE_TEXT << "Setting rate failed:" << set_rate_result
                  << NORMAL_CONSOLE_TEXT << std::endl;
        return;
    }

    // Set up callback to monitor altitude while the vehicle is in flight
    telemetry->subscribe_position([](Telemetry::Position position) {
        std::cout << TELEMETRY_CONSOLE_TEXT // set to blue
                  << "Altitude: " << position.relative_altitude_m << " m"
                  << NORMAL_CONSOLE_TEXT // set to default color again
                  << std::endl;
    });

    // Check if vehicle is ready to arm
    while (telemetry->health_all_ok() != true) {
        std::cout << "Vehicle is getting ready to arm" << std::endl;
        sleep_for(seconds(1));
    }

    // Arm vehicle
    std::cout << "Arming..." << std::endl;
    const Action::Result arm_result = action->arm();

    if (arm_result != Action::Result::Success) {
        std::cerr << ERROR_CONSOLE_TEXT << "Arming failed:" << arm_result << NORMAL_CONSOLE_TEXT
                  << std::endl;
    }

    // Take off
    std::cout << "Taking off..." << std::endl;
    const Action::Result takeoff_result = action->takeoff();

    while (takeoff_result != Action::Result::Success){}
    
    if (takeoff_result != Action::Result::Success) {
        std::cerr << ERROR_CONSOLE_TEXT << "Takeoff failed:" << takeoff_result
                  << NORMAL_CONSOLE_TEXT << std::endl;
    }

    sleep_for(seconds(5));
    
    // Initialize python
  /*   Py_OptimizeFlag = 1;
    Py_SetProgramName(L"PythonEmbeddedExample");
   */
    double lat=telemetry->position().latitude_deg;
    double longi=telemetry->position().longitude_deg;
    double latGoal=telemetry->position().latitude_deg +(double)(rand()%10)/100;
    double longiGoal=telemetry->position().longitude_deg +(double)(rand()%10)/100;
    std::vector<std::pair<double,double>> xAndy;
    for(int i =0; i < 12; i++)
     {
        latGoal=telemetry->position().latitude_deg +(double)(rand()%10)/100;
        longiGoal= telemetry->position().longitude_deg +(double)(rand()%10)/100;
        xAndy.push_back(std::make_pair(latGoal,longiGoal));
    }
        latGoal=telemetry->position().latitude_deg +(double)(rand()%10)/100;
        longiGoal= telemetry->position().longitude_deg +(double)(rand()%10)/100;


        try { //i think threading is  the issue, each thread is trying to excute and they cause the fault 9/17/20 might be print statements from python actually
                //py::scoped_interpreter guard{};

                // Disable build of __pycache__ folders
                
                std::cout<<std::endl;
                // This imports example.py from app/example.py
                // The app folder is the root folder so you don't need to specify app.example.
                // The app/example script that is being imported is from the actual build folder!
                // Cmake will copy the python scripts after you have compiled the source code.
               

                sleep_for(seconds(1));

                std::cout <<"finished initializing class" << std::endl;

                
                FollowMe::Result follow_me_result = fm->start();

                auto bat=telemetry->battery().remaining_percent;
                std::cout<<"reinit "<<std::endl;
                auto reINIT = uav.attr("reINIT")();

                auto pos = uav.attr("runForGoals")(lat,longi);
                auto newGoal =pos.cast<std::vector<double>>();
                std::cout << "First"<<newGoal[0]<< std::endl;
                std::cout << "Second"<<newGoal[0]<< std::endl;

                if (follow_me_result != FollowMe::Result::Success) {
                    // handle start failure (in this case print error)
                    std::cout << "Failed to start following  " << std::endl;
                } 
                for(int i =0; i <1; i++)
                {
                    pos = uav.attr("runForGoals")(lat,longi);
                    newGoal =pos.cast<std::vector<double>>();
                    std::cout << "First"<<newGoal[0]<< std::endl;
                    std::cout << "Second"<<newGoal[1]<< std::endl;

                    fm->set_target_location({newGoal[0],newGoal[1],10.f, 0.f, 0.f, 0.f });
                    sleep_for(seconds(4));
                    
                }


            } catch (std::exception& e) {
                std::cerr << "Something went wrong:  " << e.what() << std::endl;
               return ;
            }


        return;
}
/*
void moverand(System& system, pybind11::object & uav) //pass in a python uav reference 
{
    std::cout << "got into the function"<<std::endl;
    auto telemetry = std::make_shared<Telemetry>(system);
    auto action = std::make_shared<Action>(system);
    auto fm = std::make_shared<FollowMe>(system);
   

    // We want to listen to the altitude of the drone at 1 Hz.
    const Telemetry::Result set_rate_result = telemetry->set_rate_position(1.0);

    if (set_rate_result != Telemetry::Result::Success) {
        std::cerr << ERROR_CONSOLE_TEXT << "Setting rate failed:" << set_rate_result
                  << NORMAL_CONSOLE_TEXT << std::endl;
        return;
    }

    // Set up callback to monitor altitude while the vehicle is in flight
    telemetry->subscribe_position([](Telemetry::Position position) {
        std::cout << TELEMETRY_CONSOLE_TEXT // set to blue
                  << "Altitude: " << position.relative_altitude_m << " m"
                  << NORMAL_CONSOLE_TEXT // set to default color again
                  << std::endl;
    });

    // Check if vehicle is ready to arm
    while (telemetry->health_all_ok() != true) {
        std::cout << "Vehicle is getting ready to arm" << std::endl;
        sleep_for(seconds(1));
    }

    // Arm vehicle
    std::cout << "Arming..." << std::endl;
    const Action::Result arm_result = action->arm();

    if (arm_result != Action::Result::Success) {
        std::cerr << ERROR_CONSOLE_TEXT << "Arming failed:" << arm_result << NORMAL_CONSOLE_TEXT
                  << std::endl;
    }

    // Take off
    std::cout << "Taking off..." << std::endl;
    const Action::Result takeoff_result = action->takeoff();

    while (takeoff_result != Action::Result::Success){}
    
    if (takeoff_result != Action::Result::Success) {
        std::cerr << ERROR_CONSOLE_TEXT << "Takeoff failed:" << takeoff_result
                  << NORMAL_CONSOLE_TEXT << std::endl;
    }

    sleep_for(seconds(5));
    
    // Initialize python
    Py_OptimizeFlag = 1;
    Py_SetProgramName(L"PythonEmbeddedExample");
    
    double lat=telemetry->position().latitude_deg;
    double longi=telemetry->position().longitude_deg;
    double latGoal=telemetry->position().latitude_deg +(double)(rand()%10)/100;
    double longiGoal=telemetry->position().longitude_deg +(double)(rand()%10)/100;
    std::vector<std::pair<double,double>> xAndy;
    for(int i =0; i < 12; i++)
     {
        latGoal=telemetry->position().latitude_deg +(double)(rand()%10)/100;
        longiGoal= telemetry->position().longitude_deg +(double)(rand()%10)/100;
        xAndy.push_back(std::make_pair(latGoal,longiGoal));
    }
        latGoal=telemetry->position().latitude_deg +(double)(rand()%10)/100;
        longiGoal= telemetry->position().longitude_deg +(double)(rand()%10)/100;


        try { //i think threading is  the issue, each thread is trying to excute and they cause the fault 9/17/20 might be print statements from python actually
                //py::scoped_interpreter guard{};

                // Disable build of __pycache__ folders
                
                std::cout<<std::endl;
                // This imports example.py from app/example.py
                // The app folder is the root folder so you don't need to specify app.example.
                // The app/example script that is being imported is from the actual build folder!
                // Cmake will copy the python scripts after you have compiled the source code.
                std::cout << "Importing module..." << std::endl;
                auto example = py::module::import("mdp");

                std::cout << "Initializing class..." << std::endl;
                const auto myExampleClass = example.attr("uav");
                auto myExampleInstance = myExampleClass(longiGoal,latGoal,xAndy); 

                sleep_for(seconds(1));

                std::cout <<"finished initializing class" << std::endl;

                auto msg = myExampleInstance.attr("run")(lat,longi,latGoal,longiGoal); // Calls the getMsg
                std::cout << "Got msg back on C++ side: " << msg.cast<std::string>() << std::endl;
                FollowMe::Result follow_me_result = fm->start();

                auto bat=telemetry->battery().remaining_percent;

                if (follow_me_result != FollowMe::Result::Success) {
                    // handle start failure (in this case print error)
                    std::cout << "Failed to start following"<< std::endl;
                }
                while (1)
                {
                    lat=telemetry->position().latitude_deg;
                    longi=telemetry->position().longitude_deg;
                    msg = myExampleInstance.attr("run")(lat,longi,latGoal,longiGoal); // Calls the getMsg
                    std::cout << "Got msg back on C++ side: " << msg.cast<std::string>() << std::endl;
                    auto cmsg =msg.cast<std::string>();

                    if(cmsg =="north"){
                        fm->set_target_location({ lat+.001,longi, 50.f, 10.f, 0.f, 0.f });
                        std::cout << "north m"<< std::endl;
                    }
                    if(cmsg =="south"){
                       fm->set_target_location({ lat-.001,longi, 50.0f, -10.f, 0.f, 0.f });
                    }

                    if(cmsg =="east"){
                        fm->set_target_location({ lat,longi+.001,0.f, 0.f, 10.f, 0.f });
                    }
                    if(cmsg =="west"){
                        fm->set_target_location({ lat,longi-.001, 0.f, 0.f, -10.f, 0.f });
                    }
                    sleep_for(seconds(5));
                    
                }


            } catch (std::exception& e) {
                std::cerr << "Something went wrong: " << e.what() << std::endl;
               return ;
            }


        //return;
}
*/