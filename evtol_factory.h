#ifndef EVTOL_FACTORY
#define EVTOL_FACTORY

#include <vector>
#include <chrono>
#include <random>
#include "evtol.h"


// Maintains a list of prototype eVTOLs.
// Returns a pointer to a heap allocated copy of one of the prototypes selected at random .
class eVTOLFactory
{
public:
	eVTOLFactory()
	{
		unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
		random_engine_ = std::default_random_engine(seed);
	}

	void addPrototype(eVTOL evtol)
	{
		prototypes_.push_back(evtol);
	}

	// pick randomly one of the prototypes and return a copy of it
	eVTOL* create_eVTOL()
	{
		std::uniform_int_distribution<int> uniform_distr(0, prototypes_.size()-1);  // [0,size-1]
		int index = uniform_distr(random_engine_);
		eVTOL* evtol = new eVTOL(prototypes_[index]);
		return evtol;
	}

private:
	std::vector<eVTOL> prototypes_; // list of prototype eVTOLs
	std::default_random_engine random_engine_;  
};


void test_eVTOLFactory()
{
	using namespace std;

	eVTOLFactory factory;
	ChargingStation cs(2);
	factory.addPrototype(eVTOL(eVTOLConfiguration("one", 1, 1, 1, 1, 1, 1), &cs));
	factory.addPrototype(eVTOL(eVTOLConfiguration("two", 1, 1, 1, 1, 1, 1), &cs));
	factory.addPrototype(eVTOL(eVTOLConfiguration("three", 1, 1, 1, 1, 1, 1), &cs));
	factory.addPrototype(eVTOL(eVTOLConfiguration("four", 1, 1, 1, 1, 1, 1), &cs));
	factory.addPrototype(eVTOL(eVTOLConfiguration("five", 1, 1, 1, 1, 1, 1), &cs));

	for (int i = 0; i < 20; i++)
	{
		cout << factory.create_eVTOL()->company_name() << endl;
	}
	
}


#endif  // EVTOL_FACTORY