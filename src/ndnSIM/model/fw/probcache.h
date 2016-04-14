/*
 * probcache.h
 *
 *  Created on: Apr 14, 2016
 *      Author: chris
 */

#ifndef SRC_NDNSIM_MODEL_FW_PROBCACHE_H_
#define SRC_NDNSIM_MODEL_FW_PROBCACHE_H_

#include "ns3/ndn-forwarding-strategy.h"

namespace ns3 {
namespace ndn {
namespace fw {

class Probcache : public ForwardingStrategy {

private:
	typedef ForwardingStrategy super;

public:
	static TypeId
	GetTypeId();

	Probcache();

	virtual void
	OnInterest(Ptr<Face> face, Ptr<Interest> interest);

	virtual void
	OnData(Ptr<Face> face, Ptr<Data> data);

protected:
	virtual bool
	DoPropagateInterest(Ptr<Face> inFace, Ptr<const Interest> interest, Ptr<pit::Entry> pitEntry);
};

}



#endif /* SRC_NDNSIM_MODEL_FW_PROBCACHE_H_ */
