#pragma once

#include <vector>
#include <deque>
#include <algorithm>
#include "sim_types.h"



// Models a charging station where eVTOLs can get their batteries recharged
// the station has a fixed number of stalls for charging
// others must queue up and wait in line for next available stall
// a charging station is an ISimulationAgent and participates in a simulation
// and therefor, updates its state at each timestep of the simulation

class ChargingStation : public ISimulationAgent
{
public:
	ChargingStation(size_t max_number_charging_stations)
	{
		this->max_number_charging_devices = max_number_charging_stations;
	}

	void begin() override
	{
		// no op
	}

	void timestep_update(size_t prev_time, size_t cur_time) override
	{
		// update each device currently charging
		std::for_each(devices_charging.begin(), 
									devices_charging.end(), 
									[prev_time, cur_time](IChargeableDevice* device) { 
										device->addCharge(device->chargeRate() * (cur_time - prev_time)); 
									}
									);

		// see if any device needs to come out of charging list
		while (true)
		{
			auto itr = std::find_if(devices_charging.begin(), devices_charging.end(), [](IChargeableDevice* device) {return device->hasFullCharge(); });
			if (itr == devices_charging.end()) break;
			devices_charging.erase(itr);
		}

		// see if any devices are waiting and can go into the charging list
		while (devices_charging.size() < max_number_charging_devices && devices_waiting.size() > 0)
		{
			IChargeableDevice* device = devices_waiting.back();
			devices_waiting.pop_back();
			devices_charging.push_back(device);
		}
	}

	void addDevice(IChargeableDevice* chargeableDevice)
	{
		if (devices_charging.size() < max_number_charging_devices)
		{
			devices_charging.push_back(chargeableDevice);
		}
		else
		{
			// new devices enter the waiting queue at the front
			devices_waiting.push_front(chargeableDevice);
		}
	}

private:
	size_t max_number_charging_devices;
	std::deque<IChargeableDevice*> devices_waiting;
	std::vector<IChargeableDevice*> devices_charging;
};



void test_ChargingStation()
{
	using namespace std;

	class MockChargeableDevice : public IChargeableDevice
	{
	public:
		virtual void addCharge(double charge) override { total_charge += charge; }
		virtual double chargeRate() override { return 1; }
		virtual bool hasFullCharge() override { return total_charge >= 3; }
	private:
		double total_charge = 0;
	};

	MockChargeableDevice dev1;
	MockChargeableDevice dev2;
	MockChargeableDevice dev3;
	MockChargeableDevice dev4;

	ChargingStation cs(2);
	cs.begin();
	cs.addDevice(&dev1);
	cs.timestep_update(0, 1);
	cs.addDevice(&dev2);
	cs.timestep_update(1, 2);
	cs.addDevice(&dev3);
	cs.timestep_update(2, 3);
	cs.addDevice(&dev4);
	cs.timestep_update(3, 4);
	cs.timestep_update(4, 5);

}