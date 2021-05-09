#pragma once

#include <iostream>
#include <string>
#include <stdexcept>

#include "sim_types.h"


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

	// returns rate of charge in kWh / s
	double charge_rate()
	{
		return battery_capacity / (time_to_charge * 60 * 60);
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

//
//class eVTOL : public ISimulationAgent, public IChargeableDevice
//{
//public:
//	
//
//	void timestep_update(size_t prev_time, size_t cur_time) override
//	{
//
//	}
//
//	void addCharge(double kWh) override
//	{
//
//	}
//
//	bool hasFullCharge() override
//	{
//
//	}
//
//	double chargeRate() override
//	{
//
//	}
//
//private:
//	eVTOLConfiguration configuration;
//};





void test()
{
	using namespace std;

	try{ eVTOLConfiguration vtol_config("", 120, 320, 0.6, 1.6, 4, 0.25); } 
	catch (const std::invalid_argument& ia) { cout << ia.what() << endl; }

	try { eVTOLConfiguration vtol_config("Alpha", 0, 320, 0.6, 1.6, 4, 0.25); }
	catch (const std::invalid_argument& ia) { cout << ia.what() << endl; }
	
	try { eVTOLConfiguration vtol_config("Alpha", 120, -10, 0.6, 1.6, 4, 0.25); }
	catch (const std::invalid_argument& ia) { cout << ia.what() << endl; }

	try { eVTOLConfiguration vtol_config("Alpha", 120, 320, 0, 1.6, 4, 0.25); }
	catch (const std::invalid_argument& ia) { cout << ia.what() << endl; }

	try{ eVTOLConfiguration vtol_config("Alpha", 120, 320, 0.6, -1.6, 4, 0.25); } 
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
	cout <<  vtol_config.charge_rate();
}