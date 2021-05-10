#ifndef EVTOL
#define EVTOL

#include <iostream>
#include <string>
#include <stdexcept>
#include <random>
#include <chrono>

#include "sim_types.h"
#include "charge_station.h"


// Describes the configuration of an eVTOL
// Validates all of the entries.
// Provides some simple calculations for compound properties.
// Provides following configuration values:
//   company_name
//   cruise_speed in mph
//   battery_capacity in kWh
//   time_to_charge in hours
//   energy_use_at_cruise in kWh/mile
//   passenger_count
//   prob_fault_per_hour
//
// Usage:
//    eVTOLConfiguration vtol_config("Alpha", 120, 320, 0.6, 1.6, 4, 0.25);
//    int num_passengers = vtol_config.passenger_count();
//    double charge_rate = vtol_config.chargeRate();
struct eVTOLConfiguration
{
	eVTOLConfiguration() {
		company_name_ = "unknonw";
		cruise_speed_ = 1;
		battery_capacity_ = 1;
		time_to_charge_ = 1;
		energy_use_at_cruise_ = 1;
		passenger_count_ = 1;
		battery_capacity_ = 1;
		prob_fault_per_hour_ = 1.0;
	}
	eVTOLConfiguration(std::string company_name, double cruise_speed, double battery_capacity,
		double time_to_charge, double energy_use_at_cruise, size_t passenger_count,
		double prob_fault_per_hour = 0.25)
	{
		if (company_name == "") throw std::invalid_argument("company_name cannot be blank.");
		if (cruise_speed <= 0.0) throw std::invalid_argument("cruise_speed must be a positive number.");
		if (battery_capacity <= 0.0) throw std::invalid_argument("battery_capacity must be a positive number.");
		if (time_to_charge <= 0.0) throw std::invalid_argument("time_to_charge must be a positive number.");
		if (energy_use_at_cruise <= 0.0) throw std::invalid_argument("energy_use_at_cruise must be a positive number.");
		if (passenger_count <= 0) throw std::invalid_argument("passenger_count must be a positive number.");
		if (prob_fault_per_hour < 0.0 || prob_fault_per_hour > 1.0) throw std::invalid_argument("prob_fault_per_hour must be in [0.0, 1.0].");

		company_name_ = company_name;
		cruise_speed_ = cruise_speed;
		battery_capacity_ = battery_capacity;
		time_to_charge_ = time_to_charge;
		energy_use_at_cruise_ = energy_use_at_cruise;
		passenger_count_ = passenger_count;
		battery_capacity_ = battery_capacity;
		prob_fault_per_hour_ = prob_fault_per_hour;
	}

	std::string company_name() { return company_name_; }

	double cruise_speed() { return cruise_speed_; }

	double battery_capacity() { return battery_capacity_; }

	double time_to_charge() { return time_to_charge_; }

	double energy_use_at_cruise() { return energy_use_at_cruise_; }

	size_t passenger_count() { return passenger_count_; }

	double prob_fault_per_hour() { return prob_fault_per_hour_; }

	// returns rate of charge for the batteries in kWh / ms
	double chargeRate()
	{
		return battery_capacity_ / (time_to_charge_ * 60.0 * 60.0 * 1000.0); // converting hours to milliseconds
	}

	// returns rate of energy consumption in kWh per millisecond when flying at cruise speed
	double energyUsePerMillisecond()
	{
		return energy_use_at_cruise_ * cruise_speed_ / (60.0 * 60.0 * 1000.0);
	} 

private:
	std::string company_name_;
	double cruise_speed_;					// in mph
	double battery_capacity_;			// in kWh
	double time_to_charge_;				// in hours
	double energy_use_at_cruise_;	// in kWh/mile
	size_t passenger_count_;
	double prob_fault_per_hour_;  
};


// describes possible states of an eVTOL
enum class eVTOLState
{
	UNKNOWN,   
	FLYING,
	CHARGING,
	WAITING
};


class eVTOL : public SimulationAgent, public ChargeableDevice
{
public:
	eVTOL(const eVTOLConfiguration& config, ChargingStation* charging_station)
	{
		configuration_ = config;
		this->charging_station_ = charging_station;
		total_flight_time_ = 0.0;
		total_charge_time_ = 0.0;
		total_wait_time_ = 0.0;
		number_of_faults_ = 0;
		current_charge_ = configuration_.battery_capacity();
		state_ = eVTOLState::UNKNOWN;
		unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
		random_engine_ = std::default_random_engine(seed);
	}

	eVTOL(const eVTOL& source_evtol)
	{
		configuration_ = source_evtol.configuration_;
		charging_station_ = source_evtol.charging_station_;
		total_flight_time_ = source_evtol.total_flight_time_;
		total_charge_time_ = source_evtol.total_charge_time_;
		total_wait_time_ = source_evtol.total_wait_time_;
		current_charge_ = source_evtol.current_charge_;
		number_of_faults_ = source_evtol.number_of_faults_;
		state_ = source_evtol.state_;
		// don't copy the random engine, just create a new one
		unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
		random_engine_ = std::default_random_engine(seed);
	}

	void begin() override
	{
		state_ = eVTOLState::FLYING;
	}

	void timestepUpdate(size_t prev_time, size_t cur_time) override
	{
		if (state_ == eVTOLState::FLYING)
		{
			total_flight_time_ += (cur_time - prev_time);
			current_charge_ -= energyUsePerMillisecond() * (cur_time - prev_time);
			number_of_faults_ += didFaultOccur(cur_time - prev_time);
			// if low on battery charge, plug into charging station
			if (percentChargeRemaining() < 0.5)
			{
				state_ = eVTOLState::WAITING;
				if (charging_station_)
				{
					charging_station_->addDevice(this);
				}
			}
		}
		else if (state_ == eVTOLState::WAITING)
		{
			total_wait_time_ += (cur_time - prev_time);
		}
		else if (state_ == eVTOLState::CHARGING)
		{
			total_charge_time_ += (cur_time - prev_time);
			if (hasFullCharge())
			{
				state_ = eVTOLState::FLYING;
			}
		}
		else
		{
			// oops, something went wrong
			throw std::logic_error("VTOL in a bad state.");
		}
	}

	// returns true if a fault occured during the time interval specified
	bool didFaultOccur(size_t interval_milliseconds)
	{
		double prob_fault_per_millisecond = configuration_.prob_fault_per_hour() / (60.0 * 60.0 * 1000.0);
		double prob_fault_during_interval = prob_fault_per_millisecond * interval_milliseconds;
		std::uniform_real_distribution<double> dist(0.0, 1.0);
		double observation = dist(random_engine_);
		bool fault_occured = observation < prob_fault_during_interval;
		return fault_occured;
	}

	// charge is in kWh
	void addCharge(double charge) override
	{
		current_charge_ += charge;
		current_charge_ = std::min(current_charge_, configuration_.battery_capacity());
		if (hasFullCharge())
		{
			state_ = eVTOLState::FLYING;
		}
		else
		{
			state_ = eVTOLState::CHARGING;
		}
	}

	bool hasFullCharge() override
	{
		return current_charge_ == configuration_.battery_capacity();
	}

	// returns the name of the state
	std::string stateName()
	{
		if (state_ == eVTOLState::FLYING)
		{
			return "FLYING";
		}
		else if (state_ == eVTOLState::CHARGING)
		{
			return "CHARGING";
		}
		else if (state_ == eVTOLState::WAITING)
		{
			return "WAITING";
		}
		else
		{
			return "UNKNOWN";
		}
	}

	// return amount of charge remaining as a percent of max charge
	double percentChargeRemaining()
	{
		return current_charge_ / configuration_.battery_capacity() * 100.0;
	}

	// returns the charge rate in kWh / ms
	double chargeRate() override
	{
		return configuration_.chargeRate();
	}

	// returns the cruising speed energy use in kWh / ms
	double energyUsePerMillisecond()
	{
		return configuration_.energyUsePerMillisecond();
	}

	std::string company_name()
	{
		return configuration_.company_name();
	}

	size_t passenger_count()
	{
		return configuration_.passenger_count();
	}

	size_t cruise_speed()
	{
		return configuration_.cruise_speed();
	}

	eVTOLState state() { return state_; }

	size_t total_flight_time() { return total_flight_time_; }

	size_t total_charge_time() { return total_charge_time_; }

	size_t total_wait_time() { return total_wait_time_; }

	double current_charge() { return current_charge_; }

	size_t number_of_faults() {	return number_of_faults_; }

	eVTOLConfiguration configuration() { return configuration_; }

private:
	eVTOLConfiguration configuration_;
	size_t total_flight_time_;  // in milliseconds
	size_t total_charge_time_;  // in milliseconds
	size_t total_wait_time_;    // in milliseconds
	double current_charge_;     // in kWh
	size_t number_of_faults_;
	eVTOLState state_;
	ChargingStation* charging_station_; // where eVTOLs get their batteries recharged
	std::default_random_engine random_engine_;
};



void test_eVTOLConfiguration()
{
	using namespace std;

	try { eVTOLConfiguration vtol_config("", 120, 320, 0.6, 1.6, 4, 0.25); }
	catch (const std::invalid_argument& ia) { cout << ia.what() << endl; }

	try { eVTOLConfiguration vtol_config("Alpha", 0, 320, 0.6, 1.6, 4, 0.25); }
	catch (const std::invalid_argument& ia) { cout << ia.what() << endl; }

	try { eVTOLConfiguration vtol_config("Alpha", 120, -10, 0.6, 1.6, 4, 0.25); }
	catch (const std::invalid_argument& ia) { cout << ia.what() << endl; }

	try { eVTOLConfiguration vtol_config("Alpha", 120, 320, 0, 1.6, 4, 0.25); }
	catch (const std::invalid_argument& ia) { cout << ia.what() << endl; }

	try { eVTOLConfiguration vtol_config("Alpha", 120, 320, 0.6, -1.6, 4, 0.25); }
	catch (const std::invalid_argument& ia) { cout << ia.what() << endl; }

	try { eVTOLConfiguration vtol_config("Alpha", 120, 320, 0.6, 1.6, 0, 0.25); }
	catch (const std::invalid_argument& ia) { cout << ia.what() << endl; }

	eVTOLConfiguration vtol_config("Alpha", 120, 320, 0.6, 1.6, 4, 0.25);

	cout << vtol_config.company_name() << "  ";
	cout << vtol_config.cruise_speed() << "  ";
	cout << vtol_config.battery_capacity() << " ";
	cout << vtol_config.time_to_charge() << "  ";
	cout << vtol_config.energy_use_at_cruise() << "  ";
	cout << vtol_config.passenger_count() << "  ";
	cout << vtol_config.prob_fault_per_hour() << " ";
	cout << vtol_config.chargeRate();
}


void test_eVTOL()
{
	using namespace std;

	eVTOLConfiguration vtol_config("Alpha", 120, 320, 0.6, 1.6, 4, 0.25);
	eVTOL vtol(vtol_config, nullptr);

	cout << vtol.company_name() << endl;
	cout << (vtol.state() == eVTOLState::UNKNOWN) << endl;
	cout << vtol.percentChargeRemaining() << endl;
	cout << vtol.chargeRate() << endl;
	cout << vtol.energyUsePerMillisecond() << endl;

	vtol.begin();
	cout << (vtol.state() == eVTOLState::FLYING) << endl;
	vtol.timestepUpdate(0, 1000);
	vtol.timestepUpdate(1000, 2000);
	vtol.timestepUpdate(2000, 3000);
	cout << (vtol.state() == eVTOLState::FLYING) << endl;
	cout << vtol.percentChargeRemaining() << endl;
	vtol.timestepUpdate(3000, 30000);
	cout << vtol.percentChargeRemaining() << endl;
	vtol.timestepUpdate(30000, 100000);
	cout << vtol.percentChargeRemaining() << endl;
}


#endif  // EVTOL