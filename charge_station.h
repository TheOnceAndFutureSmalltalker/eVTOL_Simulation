#ifndef CHARGE_STATION
#define CHARGE_STATION

#include <vector>
#include <deque>
#include <algorithm>
#include "sim_types.h"



// Models a charging station where eVTOLs can get their batteries recharged.
// The station has a fixed number of bays for charging.  If all charging bays are occupied,
// an eVTOL must queue up and wait in line for next available charging bay.
// A charging station is a SimulationAgent and participates in a simulation
// and therefore, updates its state at each timestep of the simulation.
class ChargingStation : public SimulationAgent
{
public:
	ChargingStation(size_t max_number_charging_devices)
	{
		this->max_number_charging_devices_ = max_number_charging_devices;
	}

	void begin() override
	{
		// no op
	}

	void timestepUpdate(size_t prev_time, size_t cur_time) override
	{
		// update each device currently charging
		for (auto device : devices_charging_)
		{
			device->addCharge(device->chargeRate() * (cur_time - prev_time));
		}

		// See if any devices currently charging are done charging
		// and therefore need to come out of the devices_charging list 
		while (true)
		{
			auto itr = std::find_if(devices_charging_.begin(), devices_charging_.end(), 
				[](ChargeableDevice* device) {return device->hasFullCharge(); });
			if (itr == devices_charging_.end()) break;
			devices_charging_.erase(itr);
		}

		// See if any devices are waiting for a charging bay and if a charge bay is available.
		// If so, pop the next waiting device and add it to the charging devices list.
		while (devices_charging_.size() < max_number_charging_devices_ && devices_waiting_.size() > 0)
		{
			ChargeableDevice* device = devices_waiting_.back();
			devices_waiting_.pop_back();
			devices_charging_.push_back(device);
		}
	}

	// A new chargeable device is entering the charging station.
	// If there is an open charging bay, add it to the charging devices list.
	// If not, add it to the waiting queue.
	void addDevice(ChargeableDevice* chargeableDevice)
	{
		if (devices_charging_.size() < max_number_charging_devices_)
		{
			devices_charging_.push_back(chargeableDevice);
		}
		else
		{
			// new devices enter the waiting queue at the front
			devices_waiting_.push_front(chargeableDevice);
		}
	}

private:
	size_t max_number_charging_devices_;
	std::deque<ChargeableDevice*> devices_waiting_;
	std::vector<ChargeableDevice*> devices_charging_;
};



void test_ChargingStation()
{
	using namespace std;

	class MockChargeableDevice : public ChargeableDevice
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
	cs.timestepUpdate(0, 1);
	cs.addDevice(&dev2);
	cs.timestepUpdate(1, 2);
	cs.addDevice(&dev3);
	cs.timestepUpdate(2, 3);
	cs.addDevice(&dev4);
	cs.timestepUpdate(3, 4);
	cs.timestepUpdate(4, 5);

}


#endif  // CHARGE_STATION