#ifndef __DIST_H
#define __DIST_H

#include <random>
#include <stdint.h>

class Dist {
    public:
        virtual ~Dist() {};
        virtual uint64_t nextArrivalNs() = 0;
};

class ExpDist : public Dist {
    private:
        std::default_random_engine g;
        std::exponential_distribution<double> d;
        uint64_t curNs;

    public:
        ExpDist() {}
        ExpDist(double lambda, uint64_t seed, uint64_t startNs) 
            : g(seed), d(lambda), curNs(startNs) {}

        uint64_t nextArrivalNs() {
            curNs += d(g);
            return curNs;
        }
};

#endif
