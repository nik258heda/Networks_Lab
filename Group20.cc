/*
Network Topology ->  n0 -------- n1
Given,  link bandwidth -> 1Mbps
        link delay -> 10ms
*/

#include <iostream>
#include <fstream>
#include <string>
#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/tcp-header.h>
#include <ns3/ipv4-global-routing-helper.h>
#include <ns3/flow-monitor-helper.h>
#include <ns3/point-to-point-module.h>
#include <ns3/calendar-scheduler.h>
#include <ns3/flow-monitor-module.h>
#include <ns3/error-model.h>
#include <ns3/udp-header.h>
#include <ns3/enum.h>
#include <ns3/event-id.h>
#include <ns3/scheduler.h>
#include <ns3/internet-module.h>
#include <ns3/gnuplot.h>
#include <ns3/applications-module.h>
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpComparison");

AsciiTraceHelper ascii;

class MyApp:public Application{
public:
  Ptr<Socket> app_socket;
  Address app_peer;
  DataRate app_dataRate;
  EventId app_sendEvent;
  uint32_t app_pktSize;
  uint32_t app_numPkt;
  uint32_t app_packetsSent;
  bool app_running;

  MyApp(){
    app_socket = 0;
    app_dataRate = 0;
    app_pktSize = 0;
    app_numPkt = 0;
    app_packetsSent = 0;
    app_running = false;
  }
  void Setup(Ptr<Socket> socket, Address address, uint32_t pktSize, uint32_t numPkt, DataRate dataRate){
    app_socket = socket;
    app_peer = address;
    app_pktSize = pktSize;
    app_numPkt = numPkt;
    app_dataRate = dataRate;
  }
  virtual void StartApplication(){
    app_running = true;
    app_packetsSent = 0;
    app_socket->Bind();
    app_socket->Connect(app_peer);
    SendPacket();
  }
  void SendPacket(){
    Ptr<Packet> packet = Create<Packet> (app_pktSize);
    app_packetsSent++;
    app_socket->Send(packet);
    if(app_packetsSent < app_numPkt){
      ScheduleTx();
    }
  }
  void ScheduleTx(){
    if(app_running){
      Time tNext (Seconds (app_pktSize * 8 / static_cast<double> (app_dataRate.GetBitRate ())));
      app_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
  }
  virtual void StopApplication(){
    app_running = false;
    if(app_sendEvent.IsRunning()){
      Simulator::Cancel(app_sendEvent);
    }
    if(app_socket){
      app_socket->Close();
    }
  }
  ~MyApp(){
    app_socket = 0;
  }
};

void ThroughputMonitor(Ptr<OutputStreamWrapper> wrapper, FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon){
  
  Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
  std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
  double throughput=0;
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator statistics = flowStats.begin (); statistics != flowStats.end (); ++statistics)
  {
    throughput += statistics->second.rxBytes;
  }
  
  static double time = 0;
  *wrapper->GetStream() << time << "\t" << ((double)throughput/1000) << '\n';
  time += 0.05;
  Simulator::Schedule(Seconds(0.05),&ThroughputMonitor, wrapper , fmhelper, flowMon);
}

void CwndChange(Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd){
  *stream->GetStream()<<Simulator::Now().GetSeconds()<<"\t"<<((double)newCwnd/1000)<<"\n";
}

void RxDrop(Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p){
  static int i=1;
  *stream->GetStream()<<Simulator::Now().GetSeconds()<<"\t"<<i<<"\n";
  i++;
}

int main (){
    std::cout<<"Enter [1-5] to select tcp_variant:\n";
    std::cout<<"1.TCP New Reno\n";
    std::cout<<"2.TCP Hybla\n";
    std::cout<<"3.TCP Westwood\n";
    std::cout<<"4.TCP Scalable\n";
    std::cout<<"5.TCP Vegas\n";
    std::string tcp_variant;
    int option;
    std::cin>>option;
    if(option == 1){
      tcp_variant = "TcpNewReno";
      Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId()));
    }
    else if(option == 2){
      tcp_variant ="TcpHybla";
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));
    }
    else if(option == 3){
      tcp_variant ="TcpWestwood";
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
      Config::SetDefault ("ns3::TcpWestwood::FilterType", EnumValue (TcpWestwood::TUSTIN));
    }
    else if(option == 4){
      tcp_variant ="TcpScalable";
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpScalable::GetTypeId()));
    }
    else if(option == 5){
      tcp_variant ="TcpVegas";
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpVegas::GetTypeId ()));
    }
    else{
      std::cout<<"Invalid input\nExiting.....";
      exit (0);
    }
    std::cout<<"Selected algo: "<<tcp_variant<<"\n\n";
    std::string congestion_window = "cwnd_"+tcp_variant+".txt";
    std::string bytes_transferred = "bytes_"+tcp_variant+".txt";
    std::string packets_dropped = "dropped_"+tcp_variant+".txt";
    
    /* Declarations */
    NetDeviceContainer devices;
    InternetStackHelper internet;
    Ptr<MyApp> tcp_ftp_agent = CreateObject<MyApp> (); // creating TCP application at n0
    uint16_t tcp_sink_port = 1234; 
    NodeContainer nodes;
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    PointToPointHelper p2p;

    nodes.Create (2);
    std::cout<<"Nodes Created.\n";    // NS_LOG_INFO ("Created 2 nodes.");

    p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue("1500B"));
    p2p.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));
    devices = p2p.Install (nodes);
    internet.Install (nodes);
    
    Ipv4AddressHelper ipv4_address; // creating ipv4 address object
    ipv4_address.SetBase ("10.1.1.0", "255.255.255.252"); //setting ipv4 base
    Ipv4InterfaceContainer interfaces = ipv4_address.Assign (devices); // installed devices get the 2 nodes of the topology

    Ipv4GlobalRoutingHelper::PopulateRoutingTables (); // populating routing tables.

    std::cout<<"Creating Applications...\n";
    Ptr<Socket> ns3TcpSocket1 = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ()); // source at n0

    Address sinkAddress (InetSocketAddress (interfaces.GetAddress (1), tcp_sink_port));
    PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (interfaces.GetAddress (1), tcp_sink_port));
    ApplicationContainer tcp_sink_app_first = packetSinkHelper.Install (nodes.Get (1));
    tcp_sink_app_first.Start (Seconds (0.0));
    tcp_sink_app_first.Stop (Seconds (1.8));
    tcp_ftp_agent->Setup (ns3TcpSocket1, sinkAddress, 1040, 1000, DataRate ("300kbps"));
    tcp_ftp_agent->SetStartTime (Seconds (0.0));
    tcp_ftp_agent->SetStopTime (Seconds (1.8));
    nodes.Get (0)->AddApplication (tcp_ftp_agent);
    
    //Create udp applications
    uint16_t cbrPort = 1235;
    double start_time[5] = {0.2, 0.4, 0.6, 0.8, 1.0};
    double end_time[5]   = {1.8, 1.8, 1.2, 1.4, 1.6};
    for(int i=0;i<5;i++){
      uint16_t cbr_sink_port = cbrPort+i;
      Address cbr_sink_address (InetSocketAddress (interfaces.GetAddress (1), cbr_sink_port));
      PacketSinkHelper packetSinkHelper2 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), cbr_sink_port));
      ApplicationContainer cbr_sink_app = packetSinkHelper2.Install (nodes.Get (1)); //n1 as sink
      cbr_sink_app.Start (Seconds (0.0));
      cbr_sink_app.Stop (Seconds (1.8));

      // Create UDP application  at n[i]
      Ptr<MyApp> cbr_agent = CreateObject<MyApp> ();
      Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (nodes.Get (0), UdpSocketFactory::GetTypeId ()); //source at n[i]
      cbr_agent->Setup (ns3UdpSocket, cbr_sink_address, 1040, 100000, DataRate ("300Kbps"));
      cbr_agent->SetStartTime (Seconds (start_time[i]));
      cbr_agent->SetStopTime (Seconds (end_time[i])); 
      nodes.Get(0)->AddApplication (cbr_agent); 
    }
    std::cout<<"Applications created.\n";

    Ptr<OutputStreamWrapper> cwnd_data = ascii.CreateFileStream (congestion_window);
    Ptr<OutputStreamWrapper> total_bytes_data = ascii.CreateFileStream (bytes_transferred);
    Ptr<OutputStreamWrapper> dropped_packets_data = ascii.CreateFileStream (packets_dropped);
    ns3TcpSocket1->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange,cwnd_data));
    
    Ptr<RateErrorModel> error_model = CreateObject<RateErrorModel> ();// Creating error model
    error_model->SetAttribute ("ErrorRate", DoubleValue (0.0001));
    devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (error_model));
    devices.Get(1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop,dropped_packets_data));

    flowMonitor = flowHelper.InstallAll();
    ThroughputMonitor(total_bytes_data, &flowHelper, flowMonitor); //Call ThroughputMonitor Function
  
    Simulator::Stop (Seconds (1.80));
    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}

