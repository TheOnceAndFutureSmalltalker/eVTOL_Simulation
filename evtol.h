#pragma once

#include <iostream>
#include <string>
#include <stdexcept>

#include "sim_types.h"
#include "charge_station.h"


// Describes the configuration of an eVTOL
// Validates all of the entries.
// Provides some simple calculations for compound properties
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
//    int num_passengers = vtol_config.get_passenger_count();
//    double charge_rate = vtol_config.charge_rate();
struct eVTOLConfiguration
{
	eVTOLConfiguration() {
		company_name = "unknonw";
		cruise_speed = 1;
		battery_capacity = 1;
		time_to_charge = 1;
		energy_use_at_cruise = 1;
		passenger_count = 1;
		battery_capacity = 1;
		prob_fault_per_hour = 1.0;
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

		this->company_name = company_name;
		this->cruise_speed = cruise_speed;
		this->battery_capacity = battery_capacity;
		this->time_to_charge = time_to_charge;
		this->energy_use_at_cruise = energy_use_at_cruise;
		this->passenger_count = passenger_count;
		this->battery_capacity = battery_capacity;
		this->prob_fault_per_hour = prob_fault_per_hour;
	}

	std::string get_company_name() { return company_name; }

	double get_cruise_speed() { return cruise_speed; }

	double get_battery_capacity() { return battery_capacity; }

	double get_time_to_charge() { return time_to_charge; }

	double get_energy_use_at_cruise() { return energy_use_at_cruise; }

	size_t get_passenger_count() { return passenger_count; }

	double get_prob_fault_per_hour() { return prob_fault_per_hour; }

	// returns rate of charge in kWh / ms
	double charge_rate()
	{
		return battery_capacity / (time_to_charge * 60 * 60 * 1000); // converting hours to milliseconds
	}

	double energy_use_milliseconds()
	{
		return energy_use_at_cruise * cruise_speed / (60 * 60 * 1000);
	} 

private:
	std::string company_name;
	double cruise_speed;					// in mph
	double battery_capacity;			// in kWh
	double time_to_charge;				// in hours
	double energy_use_at_cruise;	// in kWh/mile
	size_t passenger_count;
	double prob_fault_per_hour;  
};


// describes possible states of an eVTOL
enum class eVTOLState
{
	UNKNOWN,
	FLYING,
	CHARGING,
	WAITING
};


class eVTOL : public ISimulationAgent, public IChargeableDevice
{
public:
	eVTOL(const eVTOLConfiguration& config, ChargingStation* charging_station)
	{
		configuration = config;
		this->charging_station = charging_station;
		total_flight_time = 0.0;
		total_charge_time = 0.0;
		total_wait_time = 0.0;
		current_charge = configuration.get_battery_capacity();
		state = eVTOLState::UNKNOWN;
	}

	eVTOL(const eVTOL& source_evtol)
	{
		configuration = source_evtol.configuration;
		charging_station = source_evtol.charging_station;
		total_flight_time = source_evtol.total_flight_time;
		total_charge_time = source_evtol.total_charge_time;
		total_wait_time = source_evtol.total_wait_time;
		current_charge = source_evtol.current_charge;
		state = source_evtol.state;
	}

	void begin() override
	{
		state = eVTOLState::FLYING;
	}

	void timestep_update(size_t prev_time, size_t cur_time) override
	{
		if (state == eVTOLState::FLYING)
		{
			total_flight_time += (cur_time - prev_time);
			current_charge -= energy_use_milliseconds() * (cur_time - prev_time);
			// if low on battery charge, plug into charging station
			if (percent_charge_remaining() < 0.5)
			{
				state = eVTOLState::WAITING;
				if (charging_station)
				{
					charging_station->addDevice(this);
					//std::cout << std::endl << "start charging " << configuration.get_company_name() << " " << (cur_time / (1000 * 60)) << std::endl;
				}
			}
		}
		else if (state == eVTOLState::WAITING)
		{
			total_wait_time += (cur_time - prev_time);
		}
		else if (state == eVTOLState::CHARGING)
		{
			total_charge_time += (cur_time - prev_time);
			if (hasFullCharge())
			{
				state = eVTOLState::FLYING;
			}
		}
		else
		{
			// oops, something went wrong
			throw std::logic_error("VTOL in a bad state.");
		}
		//std::cout << cur_time << "  " << (state == eVTOLState::FLYING ? "FLYING" : "CHARGING") << std::endl;
	}

	// charge is in kWh
	void addCharge(double charge) override
	{
		current_charge += charge;
		current_charge = std::min(current_charge, configuration.get_battery_capacity());
		if (hasFullCharge())
		{
			state = eVTOLState::FLYING;
		}
		else
		{
			state = eVTOLState::CHARGING;
		}
	}

	bool hasFullCharge() override
	{
		return current_charge == configuration.get_battery_capacity();
	}

	// return amount of charge remaining as a percent of max charge
	double percent_charge_remaining()
	{
		return current_charge / configuration.get_battery_capacity() * 100.0;
	}

	// returns the charge rate in kWh / ms
	double chargeRate() override
	{
		return configuration.charge_rate();
	}

	// returns the cruising speed energy use in kWh / ms
	double energy_use_milliseconds()
	{
		return configuration.energy_use_milliseconds();
	}

	std::string get_company_name()
	{
		return configuration.get_company_name();
	}

	size_t get_passenger_count()
	{
		return configuration.get_passenger_count();
	}

	size_t get_cruise_speed()
	{
		return configuration.get_cruise_speed();
	}


	eVTOLState get_state() { return state; }

	size_t get_total_flight_time() { return total_flight_time; }

	size_t get_total_charge_time() { return total_charge_time; }

	size_t get_total_wait_time() { return total_wait_time; }

	double get_current_charge() { return current_charge; }

	double get_faults() { return total_flight_time / (1000.0 * 60 * 60) * configuration.get_prob_fault_per_hour(); }

	eVTOLConfiguration get_configuration() { return configuration; }

private:
	eVTOLConfiguration configuration;
	size_t total_flight_time;  // in milliseconds
	size_t total_charge_time;  // in milliseconds
	size_t total_wait_time;    // in milliseconds
	double current_charge;
	eVTOLState state;
	ChargingStation* charging_station; // where eVTOLs get their batteries recharged
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

	cout << vtol_config.get_company_name() << "  ";
	cout << vtol_config.get_cruise_speed() << "  ";
	cout << vtol_config.get_battery_capacity() << " ";
	cout << vtol_config.get_time_to_charge() << "  ";
	cout << vtol_config.get_energy_use_at_cruise() << "  ";
	cout << vtol_config.get_passenger_count() << "  ";
	cout << vtol_config.get_prob_fault_per_hour() << " ";
	cout << vtol_config.charge_rate();
}


void test_eVTOL()
{
	using namespace std;

	eVTOLConfiguration vtol_config("Alpha", 120, 320, 0.6, 1.6, 4, 0.25);
	eVTOL vtol(vtol_config, nullptr);

	cout << vtol.get_company_name() << endl;
	cout << (vtol.get_state() == eVTOLState::UNKNOWN) << endl;
	cout << vtol.percent_charge_remaining() << endl;
	cout << vtol.chargeRate() << endl;
	cout << vtol.energy_use_milliseconds() << endl;

	vtol.begin();
	cout << (vtol.get_state() == eVTOLState::FLYING) << endl;
	vtol.timestep_update(0, 1000);
	vtol.timestep_update(1000, 2000);
	vtol.timestep_update(2000, 3000);
	cout << (vtol.get_state() == eVTOLState::FLYING) << endl;
	cout << vtol.percent_charge_remaining() << endl;
	vtol.timestep_update(3000, 30000);
	cout << vtol.percent_charge_remaining() << endl;
	vtol.timestep_update(30000, 100000);
	cout << vtol.percent_charge_remaining() << endl;
}