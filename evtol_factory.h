#pragma once

#include <vector>
#include <chrono>
#include <random>
#include "evtol.h"


// maintains a list of prototype eVTOLs
// returns a copy of one selected at random as a factory operation
class eVTOLFactory
{
public:
	eVTOLFactory()
	{
		unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
		random_engine = std::default_random_engine(seed);
	}

	void add_prototype(eVTOL evtol)
	{
		prototypes.push_back(evtol);
	}

	// pick randomly one of the prototypes and return a copy of it
	eVTOL create_eVTOL()
	{
		std::uniform_int_distribution<int> uniform_distr(0, prototypes.size()-1);  // [0,5]
		int index = uniform_distr(random_engine);
		return prototypes[index];
	}

private:
	std::vector<eVTOL> prototypes; // list of prototype eVTOLs
	std::default_random_engine random_engine;  
};


void test_eVTOLFactory()
{
	using namespace std;

	eVTOLFactory factory;
	ChargingStation cs(2);
	factory.add_prototype(eVTOL(eVTOLConfiguration("one", 1, 1, 1, 1, 1, 1), &cs));
	factory.add_prototype(eVTOL(eVTOLConfiguration("two", 1, 1, 1, 1, 1, 1), &cs));
	factory.add_prototype(eVTOL(eVTOLConfiguration("three", 1, 1, 1, 1, 1, 1), &cs));
	factory.add_prototype(eVTOL(eVTOLConfiguration("four", 1, 1, 1, 1, 1, 1), &cs));
	factory.add_prototype(eVTOL(eVTOLConfiguration("five", 1, 1, 1, 1, 1, 1), &cs));

	for (int i = 0; i < 20; i++)
	{
		cout << factory.create_eVTOL().get_configuration().get_company_name() << endl;
	}
	
}