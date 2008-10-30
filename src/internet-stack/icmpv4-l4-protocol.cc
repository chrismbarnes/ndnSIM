#include "icmpv4-l4-protocol.h"
#include "ipv4-interface.h"
#include "ipv4-l3-protocol.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/boolean.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Icmpv4L4Protocol");

NS_OBJECT_ENSURE_REGISTERED (Icmpv4L4Protocol);

  // see rfc 792
enum {
 ICMP_PROTOCOL = 1
};

TypeId 
Icmpv4L4Protocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Icmpv4L4Protocol")
    .SetParent<Ipv4L4Protocol> ()
    .AddConstructor<Icmpv4L4Protocol> ()
    .AddAttribute ("CalcChecksum", 
		   "Control whether the icmp header checksum is calculated and stored in outgoing icmpv4 headers",
		   BooleanValue (false),
		   MakeBooleanAccessor (&Icmpv4L4Protocol::m_calcChecksum),
		   MakeBooleanChecker ())
    ;
  return tid;
}

Icmpv4L4Protocol::Icmpv4L4Protocol ()
  : m_node (0)
{}
Icmpv4L4Protocol::~Icmpv4L4Protocol ()
{
  NS_ASSERT (m_node == 0);
}

void 
Icmpv4L4Protocol::SetNode (Ptr<Node> node)
{
  m_node = node;
}

uint16_t 
Icmpv4L4Protocol::GetStaticProtocolNumber (void)
{
  return ICMP_PROTOCOL;
}

int 
Icmpv4L4Protocol::GetProtocolNumber (void) const
{
  return ICMP_PROTOCOL;
}
void
Icmpv4L4Protocol::SendMessage (Ptr<Packet> packet, Ipv4Address dest, uint8_t type, uint8_t code)
{
  Ptr<Ipv4L3Protocol> ipv4 = m_node->GetObject<Ipv4L3Protocol> ();
  uint32_t i;
  if (!ipv4->GetIfIndexForDestination (dest, i))
    {
      NS_LOG_WARN ("drop icmp message");
      return;
    }
  Ipv4Address source = ipv4->GetAddress (i);
  SendMessage (packet, source, dest, type, code);
}

void
Icmpv4L4Protocol::SendMessage (Ptr<Packet> packet, Ipv4Address source, Ipv4Address dest, uint8_t type, uint8_t code)
{
  Ptr<Ipv4L3Protocol> ipv4 = m_node->GetObject<Ipv4L3Protocol> ();
  Icmpv4Header icmp;
  icmp.SetType (type);
  icmp.SetCode (code);
  if (m_calcChecksum)
    {
      icmp.EnableChecksum ();
    }
  packet->AddHeader (icmp);
  ipv4->Send (packet, source, dest, ICMP_PROTOCOL);
}
void 
Icmpv4L4Protocol::SendDestUnreachFragNeeded (Ipv4Header header, 
					     Ptr<const Packet> orgData, 
					     uint16_t nextHopMtu)
{
  NS_LOG_FUNCTION (this << header << *orgData << nextHopMtu);
  SendDestUnreach (header, orgData, Icmpv4DestinationUnreachable::FRAG_NEEDED, nextHopMtu);
}
void 
Icmpv4L4Protocol::SendDestUnreachPort (Ipv4Header header, 
				       Ptr<const Packet> orgData)
{
  NS_LOG_FUNCTION (this << header << *orgData);
  SendDestUnreach (header, orgData, Icmpv4DestinationUnreachable::PORT_UNREACHABLE, 0);
}
void 
Icmpv4L4Protocol::SendDestUnreach (Ipv4Header header, Ptr<const Packet> orgData, 
				   uint8_t code, uint16_t nextHopMtu)
{
  NS_LOG_FUNCTION (this << header << *orgData << (uint32_t) code << nextHopMtu);
  Ptr<Packet> p = Create<Packet> ();
  Icmpv4DestinationUnreachable unreach;
  unreach.SetNextHopMtu (nextHopMtu);
  unreach.SetHeader (header);
  unreach.SetData (orgData);
  p->AddHeader (unreach);
  SendMessage (p, header.GetSource (), Icmpv4Header::DEST_UNREACH, code);
}

void 
Icmpv4L4Protocol::SendTimeExceededTtl (Ipv4Header header, Ptr<const Packet> orgData)
{
  NS_LOG_FUNCTION (this << header << *orgData);
  Ptr<Packet> p = Create<Packet> ();
  Icmpv4TimeExceeded time;
  time.SetHeader (header);
  time.SetData (orgData);
  p->AddHeader (time);
  SendMessage (p, header.GetSource (), Icmpv4Header::TIME_EXCEEDED, Icmpv4TimeExceeded::TIME_TO_LIVE);
}

void
Icmpv4L4Protocol::HandleEcho (Ptr<Packet> p,
			      Icmpv4Header header, 
			      Ipv4Address source,
			      Ipv4Address destination)
{
  NS_LOG_FUNCTION (this << p << header << source << destination);

  Ptr<Packet> reply = Create<Packet> ();
  Icmpv4Echo echo;
  p->RemoveHeader (echo);
  reply->AddHeader (echo);
  SendMessage (reply, destination, source, Icmpv4Header::ECHO_REPLY, 0);
}
void
Icmpv4L4Protocol::Forward (Ipv4Address source, Icmpv4Header icmp,
			   uint32_t info, Ipv4Header ipHeader,
			   const uint8_t payload[8])
{
  Ptr<Ipv4L3Protocol> ipv4 = m_node->GetObject<Ipv4L3Protocol> ();
  Ptr<Ipv4L4Protocol> l4 = ipv4->GetProtocol (ipHeader.GetProtocol ());
  if (l4 != 0)
    {
      l4->ReceiveIcmp (source, ipHeader.GetTtl (), icmp.GetType (), icmp.GetCode (),
		       info, ipHeader.GetSource (), ipHeader.GetDestination (), payload);
    }
}
void
Icmpv4L4Protocol::HandleDestUnreach (Ptr<Packet> p,
				     Icmpv4Header icmp, 
				     Ipv4Address source,
				     Ipv4Address destination)
{
  NS_LOG_FUNCTION (this << p << icmp << source << destination);

  Icmpv4DestinationUnreachable unreach;
  p->PeekHeader (unreach);
  uint8_t payload[8];
  unreach.GetData (payload);
  Ipv4Header ipHeader = unreach.GetHeader ();
  Forward (source, icmp, unreach.GetNextHopMtu (), ipHeader, payload);
}
void
Icmpv4L4Protocol::HandleTimeExceeded (Ptr<Packet> p,
				      Icmpv4Header icmp, 
				      Ipv4Address source,
				      Ipv4Address destination)
{
  NS_LOG_FUNCTION (this << p << icmp << source << destination);

  Icmpv4TimeExceeded time;
  p->PeekHeader (time);
  uint8_t payload[8];
  time.GetData (payload);
  Ipv4Header ipHeader = time.GetHeader ();
  // info field is zero for TimeExceeded on linux
  Forward (source, icmp, 0, ipHeader, payload);
}

enum Ipv4L4Protocol::RxStatus
Icmpv4L4Protocol::Receive(Ptr<Packet> p, 
			  Ipv4Address const &source,
			  Ipv4Address const &destination,
			  Ptr<Ipv4Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << p << source << destination << incomingInterface);

  Icmpv4Header icmp;
  p->RemoveHeader (icmp);
  switch (icmp.GetType ()) {
  case Icmpv4Header::ECHO:
    HandleEcho (p, icmp, source, destination);
    break;
  case Icmpv4Header::DEST_UNREACH:
    HandleDestUnreach (p, icmp, source, destination);
    break;
  case Icmpv4Header::TIME_EXCEEDED:
    HandleTimeExceeded (p, icmp, source, destination);
    break;
  default:
    NS_LOG_DEBUG (icmp << " " << *p);
    break;
  }
  return Ipv4L4Protocol::RX_OK;
}
void 
Icmpv4L4Protocol::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_node = 0;
  Ipv4L4Protocol::DoDispose ();
}

} // namespace ns3