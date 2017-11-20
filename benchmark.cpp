#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TEnv.h"
#include "TRandom3.h"

#include "TTreePerfStats.h"
#include "TStopwatch.h"

#include <getopt.h>

#include <iostream>
#include <vector>

struct {
	UInt_t event;
	Double_t energy;
	UInt_t nTimes;
	std::vector<Double_t> times;
	UInt_t nIndices;
	std::vector<UInt_t> indices;
} treeData;

void writeTree(int seed, unsigned int events, TString rootFileName) {
	TRandom3 rnd(seed);

	auto tree = new TTree("t", "t");

	tree->Branch("event", &(treeData.event), "event/i");

	tree->Branch("energy", &(treeData.energy), "energy/D");

	treeData.times.resize(2000);
	tree->Branch("nTimes", &(treeData.nTimes), "nTimes/i");
	tree->Branch("times", treeData.times.data(), "times[nTimes]/D");

	treeData.indices.resize(2000);
	tree->Branch("nIndices", &(treeData.nIndices), "nIndices/i");
	tree->Branch("indices", treeData.indices.data(), "indices[nIndices]/i");

	for (unsigned int i=0; i<events; ++i) {
		treeData.event = i;

		treeData.energy = rnd.Rndm()*1000.;

		treeData.nTimes = 1 + floor(rnd.Rndm()*1000.);
		for (unsigned int n=0; n < treeData.nTimes; ++n) {
			treeData.times[n] = 1 + rnd.Rndm()*1000. - 500.;
			//times[n] = 1000.5 + nTimes;
		}

		treeData.nIndices = 1 + floor(rnd.Rndm()*1000.);
		for (unsigned int n=0; n < treeData.nIndices; ++n) {
			treeData.indices[n] = 1 + floor(rnd.Rndm()*1000.);
			//indices[n] = 1 + n;
		}
		tree->Fill();
	}

	tree->Write();

	delete tree;

	std::cout << "Wrote " << events << " entries." << std::endl;
}

void readTree(TFile* inFile, TString rootFileName) {
	TTree* tree = nullptr;
	inFile->GetObject("t", tree);

	if (tree == nullptr) {
		gROOT->Fatal("readTree", "Could not find tree in file!");
	}

	auto psRead = new TTreePerfStats("readPerf", tree);

	treeData.times.resize(2000);
	treeData.indices.resize(2000);

	tree->SetBranchAddress("event", &(treeData.event));
	tree->SetBranchAddress("energy", &(treeData.energy));
	tree->SetBranchAddress("nTimes", &(treeData.nTimes));
	tree->SetBranchAddress("times", treeData.times.data());
	tree->SetBranchAddress("nIndices", &(treeData.nIndices));
	tree->SetBranchAddress("indices", treeData.indices.data());

	auto entries = tree->GetEntriesFast();
	std::cout << "Reading " << entries  << " entries." << std::endl;
	for (decltype(entries) entry = 0; entry < entries; ++entry) {
		tree->GetEntry(entry);
	}

	rootFileName.ReplaceAll(".root", "_readPerf.root");
	psRead->SaveAs(rootFileName);
	psRead->Print();

	delete psRead;
	delete tree;
}

namespace longParameters {
        enum _longParameters {
		NONE,
		ASYNC_READ,
		NO_ASYNC_READ,
		ASYNC_PREFETCH,
		NO_ASYNC_PREFETCH,
		TREECACHE_PREFILL,
		NO_TREECACHE_PREFILL,
		TREECACHE_SIZE,
        };
}

int main(int argc, char* argv[]) {

	bool readMode = false;
	bool writeMode = false;
	unsigned long seed = 42;
	unsigned long events = 600000;

	const struct option long_options[] = {
		{"async-read",			no_argument,		nullptr,	longParameters::ASYNC_READ},
		{"no-async-read",		no_argument,		nullptr,	longParameters::NO_ASYNC_READ},
		{"async-prefetch",		no_argument,		nullptr,	longParameters::ASYNC_PREFETCH},
		{"no-async-prefetch",		no_argument,		nullptr,	longParameters::NO_ASYNC_PREFETCH},
		{"treecache-prefill",		no_argument,		nullptr,	longParameters::TREECACHE_PREFILL},
		{"no-treecache-prefill",	no_argument,		nullptr,	longParameters::NO_TREECACHE_PREFILL},
		{"treecache-size",		required_argument,	nullptr,	longParameters::TREECACHE_SIZE},
		{nullptr,			0,			nullptr,	longParameters::NONE}
	};

	{
		int opt = 0;
		int option_index = 0;
		while ((opt = getopt_long(argc, argv, "rws:e:", long_options, &option_index)) != -1) {
			switch (opt) {
				case 'r': {
					readMode = true;
				}
				break;
				case 'w': {
					writeMode = true;
				}
				break;
				case longParameters::ASYNC_READ: {
					gEnv->SetValue("TFile.AsyncReading", 1);
				}
				break;
				case longParameters::NO_ASYNC_READ: {
					gEnv->SetValue("TFile.AsyncReading", 0);
				}
				break;
				case longParameters::ASYNC_PREFETCH: {
					gEnv->SetValue("TFile.AsyncPrefetching", 1);
				}
				break;
				case longParameters::NO_ASYNC_PREFETCH: {
					gEnv->SetValue("TFile.AsyncPrefetching", 0);
				}
				break;
				case longParameters::TREECACHE_PREFILL: {
					gEnv->SetValue("TTreeCache.Prefill", 1);
				}
				break;
				case longParameters::NO_TREECACHE_PREFILL: {
					gEnv->SetValue("TTreeCache.Prefill", 0);
				}
				break;
				case longParameters::TREECACHE_SIZE: {
					gEnv->SetValue("TTreeCache.Size", optarg);
				}
				break;
				case 's': {
					char *endptr;
					seed = strtoul(optarg, &endptr, 10);
					if (errno == ERANGE) {
						perror("strtoul");
						exit(1);
					}
				}
				break;
				case 'e': {
					char *endptr;
					events = strtoul(optarg, &endptr, 10);
					if (errno == ERANGE) {
						perror("strtoul");
						exit(1);
					}
				}
				break;
				default:
					std::cerr << "Error in argument parsing!" << std::endl;
					exit(1);
			}
		}
	}

	TString filename;
	filename.Form("test_%010lu.root", seed);

	TStopwatch timer;

	if (writeMode) {
		timer.Start();
		auto outFile = TFile::Open(filename, "RECREATE", "tree", 0);
		writeTree(seed, events, filename);
		outFile->Close();
		delete outFile;
		timer.Stop();
		std::cout << "Write CpuTime: "  << timer.CpuTime() << std::endl;
		std::cout << "Write RealTime: " << timer.RealTime() << std::endl;
		timer.Print("u");
	}

	if (readMode) {
		timer.Start();
		auto inFile = TFile::Open(filename, "READ");
		if (inFile == nullptr) {
			std::cerr << "Error opening file '" << filename << "' for reading!" << std::endl;
			exit(1);
		}
		readTree(inFile, filename);
		inFile->Close();
		delete inFile;
		timer.Stop();
		std::cout << "Read CpuTime: "  << timer.CpuTime() << std::endl;
		std::cout << "Read RealTime: " << timer.RealTime() << std::endl;
		timer.Print("u");
	}
}
