// Network topology
//
//       n0 ----------- n1
//            1 Mbps
//             10 ms
//
// - Flow from n0 to n1 using BulkSendApplication.

#include <iostream>
#include <fstream>
#include <string>

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/flow-monitor-helper.h>
#include <ns3/point-to-point-module.h>
#include <ns3/applications-module.h>
#include <ns3/flow-monitor-module.h>
#include <ns3/error-model.h>
#include <ns3/tcp-header.h>
#include <ns3/udp-header.h>
#include <ns3/enum.h>
#include <ns3/event-id.h>
#include <ns3/ipv4-global-routing-helper.h>
#include <ns3/scheduler.h>
#include <ns3/calendar-scheduler.h>
#include <ns3/gnuplot.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpComparision");

AsciiTraceHelper ascii;

class MyApp:public Application{
public:
  MyApp();
  virtual ~MyApp();
  void Setup(Ptr<Socket> socket, Address address, uint32_t pktSize, uint32_t numPkt, DataRate dataRate);
  virtual void StartApplication();
  virtual void StopApplication();
  void ScheduleTx();
  void SendPacket();

  Ptr<Socket> app_socket;
  Address app_peer;
  uint32_t app_pktSize;
  uint32_t app_numPkt;
  DataRate app_dataRate;
  EventId app_sendEvent;
  bool app_running;
  uint32_t app_packetsSent;
};

MyApp::MyApp()
  : app_socket(0),
    app_peer(),
    app_pktSize(0),
    app_numPkt(0),
    app_dataRate(0),
    app_sendEvent(),
    app_running(false),
    app_packetsSent(0)
{}

MyApp::~MyApp(){
  app_socket = 0;
}

void MyApp::Setup(Ptr<Socket> socket, Address address, uint32_t pktSize, uint32_t numPkt, DataRate dataRate){
  app_socket = socket;
  app_peer = address;
  app_pktSize = pktSize;
  app_numPkt = numPkt;
  app_dataRate = dataRate;
}

void MyApp::StartApplication(){
  app_running = true;
  app_packetsSent = 0;
  app_socket->Bind();
  app_socket->Connect(app_peer);
  SendPacket();
}

void MyApp::StopApplication(){
  app_running = false;
  if(app_sendEvent.IsRunning()){
    Simulator::Cancel(app_sendEvent);
  }
  if(app_socket){
    app_socket->Close();
  }
}

void MyApp::SendPacket(){
  Ptr<Packet> packet = Create<Packet> (app_pktSize);
  app_socket->Send(packet);
  if(++app_packetsSent < app_numPkt){
    ScheduleTx();
  }
}

void MyApp::ScheduleTx(){
  if(app_running){
    Time tNext (Seconds (app_pktSize * 8 / static_cast<double> (app_dataRate.GetBitRate ())));
    app_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
   }
}

/*--------------------class definitions over----------------------*/

//to keep track of changes in congestion window, using callbacks  from TCP when window is changed
static void CwndChange(Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd){
  *stream->GetStream()<<Simulator::Now().GetSeconds()<<"\t"<<((double)newCwnd/1000)<<"\n";
}

static void RxDrop(Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p){
  static int i=1;
  *stream->GetStream()<<Simulator::Now().GetSeconds()<<"\t"<<i<<"\n";
  i++;
}

void ThroughputMonitor(Ptr<OutputStreamWrapper> stream, FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon){
  static double time = 0;
  double localThrou=0;
  std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
  Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
  {
    localThrou += stats->second.rxBytes;
  }
  *stream->GetStream() << time << "\t" << ((double)localThrou/1000) << '\n';
  time += 0.05;
  Simulator::Schedule(Seconds(0.05),&ThroughputMonitor, stream , fmhelper, flowMon);
}
/*------------------------ Helper functions defined --------------------*/
int
main (int argc, char *argv[])
{
    std::string tcp_variant;
    int option;
    std::cout<<"Enter [1-5] for TCP variant:\n1.TCP New Reno\n2.TCP Hybla\n3.TCP Westwood\n4.TCP Scalable\n5.TCP Vegas\n";
    std::cin>>option;
    switch(option)
    {
      case 1:
      tcp_variant = "TcpNewReno";
      Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId()));
      break;

      case 2:
      tcp_variant ="TcpHybla";
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));
      break;

      case 3:
      tcp_variant ="TcpWestwood";
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
      Config::SetDefault ("ns3::TcpWestwood::FilterType", EnumValue (TcpWestwood::TUSTIN));
      break;

      case 4:
      tcp_variant ="TcpScalable";
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpScalable::GetTypeId()));
      break;

      case 5:
      tcp_variant ="TcpVegas";
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpVegas::GetTypeId ()));
      break;

      default:
      fprintf (stderr, "Invalid TCP version\n");
      exit (1);
    }
  
  std::string a_s = "bytes_"+tcp_variant+".dat";
  std::string b_s = "dropped_"+tcp_variant+".dat";
  std::string c_s = "cwnd_"+tcp_variant+".dat";

  // Create file streams for data storage
  Ptr<OutputStreamWrapper> total_bytes_data = ascii.CreateFileStream (a_s);
  Ptr<OutputStreamWrapper> dropped_packets_data = ascii.CreateFileStream (b_s);
  Ptr<OutputStreamWrapper> cwnd_data = ascii.CreateFileStream (c_s);
    
  // creating nodes
  NodeContainer nodes;
  nodes.Create (2);
  NS_LOG_INFO ("Created 2 nodes.");

  //NodeContainer n0n1 = NodeContainer(nodes.Get(0), nodes.Get(1));

  PointToPointHelper pointToPoint;
  pointToPoint.SetQueue ("ns3::DropTailQueue");
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));

  NS_LOG_INFO ("P2P link created....");
  NS_LOG_INFO ("Bandwidth : 1Mbps");
  NS_LOG_INFO ("Delay : 10ms");
  NS_LOG_INFO ("Queue Type : DropTailQueue");

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);
  NS_LOG_INFO ("Installing net device to nodes, adding MAC Address and Queue.");

  // Install the internet stack on the nodes
  InternetStackHelper internet;
  internet.Install (nodes);

  // Create error model
  Ptr<RateErrorModel> error_model = CreateObject<RateErrorModel> ();
  error_model->SetAttribute ("ErrorRate", DoubleValue (0.0001));
  devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (error_model));
  
  // We've got the "hardware" in place.  Now we need to add IP addresses.
  Ipv4AddressHelper ipv4_address;
  ipv4_address.SetBase ("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces = ipv4_address.Assign (devices);
  NS_LOG_INFO ("Assigned IP base addresses to the nodes");

  // Turn on global static routing so we can actually be routed across the network.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  NS_LOG_INFO ("Create Applications.");

  uint16_t tcp_sink_port = 4641;
  Address sinkAddress (InetSocketAddress (interfaces.GetAddress (1), tcp_sink_port));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (interfaces.GetAddress (1), tcp_sink_port));
  ApplicationContainer tcp_sink_app_first = packetSinkHelper.Install (nodes.Get (1));
  tcp_sink_app_first.Start (Seconds (0.0));
  tcp_sink_app_first.Stop (Seconds (1.8));

  Ptr<Socket> ns3TcpSocket1 = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ()); // source at no

  
  // create TCP application at n0
  Ptr<MyApp> tcp_ftp_agent = CreateObject<MyApp> ();
  tcp_ftp_agent->Setup (ns3TcpSocket1, sinkAddress, 1040, 1000, DataRate ("300kbps"));
  nodes.Get (0)->AddApplication (tcp_ftp_agent);
  tcp_ftp_agent->SetStartTime (Seconds (0.0));
  tcp_ftp_agent->SetStopTime (Seconds (1.8));

  

  //Create udp applications
  uint16_t cbrPort = 4564;
  double startTimes[5] = {0.2, 0.4, 0.6, 0.8, 1.0};
  double endTimes[5]   = {1.8, 1.8, 1.2, 1.4, 1.6};
  for(int i=0;i<5;i++){
    uint16_t cbr_sink_port_first = cbrPort+i;
    Address cbr_sink_address_1 (InetSocketAddress (interfaces.GetAddress (1), cbr_sink_port_first));
    PacketSinkHelper packetSinkHelper2 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), cbr_sink_port_first));
    ApplicationContainer cbr_sink_app_first = packetSinkHelper2.Install (nodes.Get (1)); //n1 as sink
    cbr_sink_app_first.Start (Seconds (0.0));
    cbr_sink_app_first.Stop (Seconds (1.8));

    Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (nodes.Get (0), UdpSocketFactory::GetTypeId ()); //source at n0

    // Create UDP application  at n0
    Ptr<MyApp> first_cbr_agent = CreateObject<MyApp> ();
    first_cbr_agent->Setup (ns3UdpSocket, cbr_sink_address_1, 1040, 100000, DataRate ("300Kbps"));
    nodes.Get(0)->AddApplication (first_cbr_agent);
    first_cbr_agent->SetStartTime (Seconds (startTimes[i]));
    first_cbr_agent->SetStopTime (Seconds (endTimes[i]));  
  }

  NS_LOG_INFO ("Applications created.");
  // Trace CongestionWindow
  ns3TcpSocket1->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange,cwnd_data));
  devices.Get(1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop,dropped_packets_data));

/*--------------Application creation ends----------------*/

  //Configuring Analysis tools
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();
  
  ThroughputMonitor(total_bytes_data, &flowHelper, flowMonitor); //Call ThroughputMonitor Function
  
    Simulator::Stop (Seconds (1.80));
    Simulator::Run ();
   // Get the stats from Flow Monitor
   Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats ();
    std::cout << std::endl << "*** Flow monitor statistics ***" << std::endl;
    std::cout << "  Tx Packets:   " << stats[1].txPackets << std::endl;
    std::cout << "  Tx Bytes:   " << stats[1].txBytes << std::endl;
    std::cout << "  Offered Load: " << stats[1].txBytes * 8.0 / (stats[1].timeLastTxPacket.GetSeconds () - stats[1].timeFirstTxPacket.GetSeconds ()) / 1000000 << " Mbps" << std::endl;
    std::cout << "  Rx Packets:   " << stats[1].rxPackets << std::endl;
    std::cout << "  Rx Bytes:   " << stats[1].rxBytes<< std::endl;
    std::cout << "  Throughput: " << stats[1].rxBytes * 8.0 / (stats[1].timeLastRxPacket.GetSeconds () - stats[1].timeFirstRxPacket.GetSeconds ()) / 1000000 << " Mbps" << std::endl;
    std::cout << "  Mean delay:   " << stats[1].delaySum.GetSeconds () / stats[1].rxPackets << std::endl;
    std::cout << "  Mean jitter:   " << stats[1].jitterSum.GetSeconds () / (stats[1].rxPackets - 1) << std::endl;
    flowMonitor->SerializeToXmlFile("data.flowmon", true, true);
  Simulator::Destroy ();

  return 0;
}
