#ifndef SIM_TIMER
#define SIM_TIMER


#include <iostream>
#include <functional>
#include <chrono>
#include <stdexcept>


// Provides timesteps for a simulation.
// Runs for a predetermined amount of simulation time.
// Runs at a configured time compression ratio of simulation time to real time.
// Fires off an event at each timestep by calling a function with signature:
//     void(size_t prev_time, size_t cur_time)
// where both arguments are the time since start of simulation as measured in milliseconds
// prev_time is provided so client code can calculate a change in time if necessary
// Usage:
//     auto event_handler = [](size_t prev_time, size_t cur_time) {};
//     SimulationEventTimer timer(1000, event_handler, 1, 1);
//     timer.start();
class SimulationEventTimer
{
public:
	SimulationEventTimer(
		size_t timestep_size_milliseconds_, 
		std::function<void(unsigned long long, unsigned long long)> timestep_event,
		size_t total_simulation_time_minutes_, 
		size_t simulation_to_real_time_ = 1)
	{
		if (timestep_size_milliseconds_ == 0) throw std::invalid_argument("timestep_size_milliseconds must be greater than 0.");
		if(!timestep_event) throw std::invalid_argument("timestep_event function is required.");
		if (simulation_to_real_time_ == 0) throw std::invalid_argument("simulation_to_real_time must be greater than 0.");

		this->timestep_size_milliseconds_ = timestep_size_milliseconds_;
		this->timestep_event_ = timestep_event;
		this->total_simulation_time_minutes_ = total_simulation_time_minutes_;
		this->simulation_to_real_time_ = simulation_to_real_time_;
	}

	// starts the timer.
	void start()
	{
		// timer reports time in milliseconds but operates in microseconds for precison purposes
		std::chrono::microseconds total_simulation_time = std::chrono::seconds(total_simulation_time_minutes_ * 60);
		std::chrono::microseconds current_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());;
		std::chrono::microseconds next_update_time = current_time;
		std::chrono::microseconds stop_time = current_time + total_simulation_time / simulation_to_real_time_;
		std::chrono::microseconds update_interval = std::chrono::microseconds(1000) * timestep_size_milliseconds_ / simulation_to_real_time_; ///(1000000us / simulation_to_real_time)
		int update_count = 0;

		while (current_time < stop_time)
		{
			if (current_time >= next_update_time)
			{
				long prev_simulation_time = update_count++ * timestep_size_milliseconds_;
				long cur_simulation_time = update_count * timestep_size_milliseconds_;
				timestep_event_(prev_simulation_time, cur_simulation_time);
				next_update_time += update_interval;
			}
			current_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
		}
	}

	double totalSimulationTimeInRealMinutes()
	{
		return total_simulation_time_minutes_ * 1.0 / simulation_to_real_time_;
	}

private:
	std::function<void(unsigned long long, unsigned long long)> timestep_event_;  // function that gets called at each timestep
	size_t simulation_to_real_time_;				// how many units of simulation time passes for every 1 unit of real time
	size_t total_simulation_time_minutes_;	// total simulation time in minutes
	size_t timestep_size_milliseconds_;			// time between timesteps in milliseconds of simulation time
};




void test_SimulationEventTimer()
{
	using namespace std;
	auto event_handler = [](unsigned long long prev_time, unsigned long long cur_time) {
		cout << prev_time << "  " << cur_time << endl;
	};

	try
	{
		SimulationEventTimer t(0, event_handler, 1, 1);
	}
	catch (const std::invalid_argument& ia)
	{
		cout << ia.what() << endl;
	}

	try
	{
		SimulationEventTimer t(1, event_handler, 1, 0);
	}
	catch (const std::invalid_argument& ia)
	{
		cout << ia.what() << endl;
	}

	SimulationEventTimer timer(1000, event_handler, 1, 1);
	//timer.start();

}


#endif  // SIM_TIMER