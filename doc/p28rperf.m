#!/usr/bin/octave

graphics_toolkit ("gnuplot");

global const_g   = 9.80665;                     % gravity const.
global const_m   = 28.9644;                     % Molecular Weight of air
global const_r   = 8.31432;                     % gas const. for air
global const_l0  = 6.5/1000;                    % Lapse rate first layer
global const_gmr = (const_g*const_m/const_r);   % Hydrostatic constant
global degc_to_kelvin = 273.15;
global std_sealevel_temperature = 15 + degc_to_kelvin;
global std_sealevel_pressure = 1013.25;
global std_sealevel_density = const_m / const_r * std_sealevel_pressure / std_sealevel_temperature;

global ft_to_m = 0.3048;

function [p t rho] = icaoatmo(alt)
  global ft_to_m;
  global std_sealevel_temperature;
  global std_sealevel_pressure;
  global const_l0;
  global const_gmr;
  global const_m;
  global const_r;
  alt = alt * ft_to_m;
  t = std_sealevel_temperature - const_l0 * alt;
  p = std_sealevel_pressure * (t ./ std_sealevel_temperature) .^ (const_gmr * 1e-3 / const_l0);
  rho = (const_m/const_r*0.1) * p ./ t;
endfunction

global hp_to_ftlbfps=550;
global mps_to_mph=2.2369363;
global mph_to_ftps=1.4666667;
global mph_to_kts=0.86897624;
global nmi_to_ft=6076.1155;
global mile_to_ft=5280;
global kgpm3_to_slugspft3=0.0019403203;
global rho0=0.002377;         # Sea level density in slugs/ft^3

S=160;                 # Wing area, ft^2
B=30;                  # Wing Span, ft
A=B^2/S;               # Wing Aspect Ratio
P0=200*hp_to_ftlbfps;  # Sea level power
n0=2700/60;            # RPM for power
M0=P0/2/pi/n0;         # Engine MSL torque
C=0.13;                # Engine altitude drop-off
CRPM=1.13;             # Engine RPM drop-off
d=6.333;               # Propeller diameter ft
#CD0=0.030662;          # Drag
CD0=0.029;             # Drag
CD0geardown=0.055;     # Drag, Gear Down
#ee=0.60648;            # Airframe efficiency
ee=0.72;
#ee=0.65;
#AF=80;                 # Propeller Activity Factor
AF=167.7;              # Propeller Activity Factor
blades=2;              # Prop Blades
pitchlim=[14 29];      # Prop Blades Pitch Limits
vrot=70;               # Rotate Speed; manual says 60-70MPH
g=9.8066;
# Rolling friction coefficient is typically:
# Dry concrete/Asphalt - 0.02
# Hard turf/Gravel     - 0.04
# Short, dry grass     - 0.05
# Long grass           - 0.10
# Soft Ground          - 0.10 to 0.30
rollfriction=0.02;
# estimate of flaps drag and oswald efficiency; from Roskam
ewing=.85;
efuselageother=1/(1/ee-1/ewing);
eeflaps10=1/(1/efuselageother+1/ewing/1.075);
eeflaps25=1/(1/efuselageother+1/ewing/1.12);
eeflaps40=1/(1/efuselageother+1/ewing/1.13);
CD0geardownflaps10=CD0geardown+0.01;
CD0geardownflaps25=CD0geardown+0.035;
CD0geardownflaps40=CD0geardown+0.06;

alt=0:100:15000;
[p t rho]=icaoatmo(alt);
figure("visible","off");
clf;
ax=plotyy(alt,p,alt,rho,@plot,@plot);
title("ICAO Standard Atmosphere");
xlabel("Altitude [ft]");
ylabel(ax(1), "Pressure [hPa]");
ylabel(ax(2), "Density [kg/m^3]");
grid on;
#set(ax(1),"ylim",[0 1100]);
#set(ax(2),"ylim",[0 1.3]);
set(ax(1),"ycolor",[0 0 0]);
set(ax(2),"ycolor",[0 0 0]);
print("p28ratmo.eps","-depsc2","-portrait","-color");

io360altperf=[0 200 176 0 ;
	      1000 194 170 0 ;
	      2000 187 164 0 ;
	      3000 181 159 136 ;
	      4000 174 153 131 ;
	      5000 168 148 126 ;
	      6000 162 142 122 ;
	      7000 156 137 117 ;
	      8000 150 132 113 ;
	      9000 145 127 108 ;
	      10000 140 123 105 ;
	      11000 135 118 100 ;
	      12000 130 114 97 ;
	      13000 125 109 93 ;
	      14000 120 105 89 ;
	      15000 116 100 86 ;
	      16000 110 96 82 ;
	      17000 106 93 79 ;
	      18000 102 89 76 ];
alt=0:100:18000;
[p t rho]=icaoatmo(alt);
clf;
plot(io360altperf(io360altperf(:,2)>0,2),io360altperf(io360altperf(:,2)>0,1),"+1;RPM=2700;",...
     io360altperf(io360altperf(:,3)>0,3),io360altperf(io360altperf(:,3)>0,1),"+2;RPM=2400;",...
     io360altperf(io360altperf(:,4)>0,4),io360altperf(io360altperf(:,4)>0,1),"+3;RPM=2100;",...
     P0/hp_to_ftlbfps*(rho/rho(1)-C)/(1-C),alt,"-1;RPM=2700;",...
     P0/hp_to_ftlbfps*(rho/rho(1)-C)/(1-C)*(2400/2700)^1.13,alt,"-2;RPM=2400;",...
     P0/hp_to_ftlbfps*(rho/rho(1)-C)/(1-C)*(2100/2700)^1.13,alt,"-3;RPM=2100;");
title("IO-360-C Engine Full Throttle SHP versus Altitude");
xlabel("Shaft Horse Power");
ylabel("Altitude [ft]");
grid on;
axis([60 200 0 18000]);
print("p28rio360pwr.eps","-depsc2","-portrait","-color");

clf;
[p1 t1 rho1]=icaoatmo(io360altperf(:,1));
RPMPWR=zeros(1,3);
RPMC=zeros(1,3);
for tt=1:3;
  ttt=io360altperf(:,tt+1)>0;
  x=polyfit(rho1(ttt)/rho1(1),io360altperf(ttt,tt+1),1);
  RPMPWR(tt)=sum(x);
  x=x/sum(x);
  RPMC(tt)=(x(1)-1)/x(1);
endfor;
plot(rho1(io360altperf(:,2)>0)/rho1(1),io360altperf(io360altperf(:,2)>0,2),"+1;RPM=2700;",...
     rho1(io360altperf(:,3)>0)/rho1(1),io360altperf(io360altperf(:,3)>0,3),"+2;RPM=2400;",...
     rho1(io360altperf(:,4)>0)/rho1(1),io360altperf(io360altperf(:,4)>0,4),"+3;RPM=2100;",...
     rho/rho(1),P0/hp_to_ftlbfps*(rho/rho(1)-C)/(1-C),"-1;RPM=2700;",...
     rho/rho(1),P0/hp_to_ftlbfps*(rho/rho(1)-C)/(1-C)*(2400/2700)^1.13,"-2;RPM=2400;",...
     rho/rho(1),P0/hp_to_ftlbfps*(rho/rho(1)-C)/(1-C)*(2100/2700)^1.13,"-3;RPM=2100;");
title("IO-360-C Engine Full Throttle SHP versus Altitude");
xlabel("Relative Density [relative]");
ylabel("Shaft Horse Power");
grid on;
axis([0.5 1 60 200]);
legend("location","northwest");
print("p28rio360pwrdensity.eps","-depsc2","-portrait","-color");

W=2600;
VXManual=80;
t=0:0.1:18;
vc=zeros(1,length(t));
D=zeros(1,length(t));
T=zeros(1,length(t));
efficiency=zeros(1,length(t));
pitch=zeros(1,length(t));
dist=zeros(1,length(t));
for tt=1:length(t);
  D(tt)=0.5*CD0geardown*(vc(tt)*mph_to_ftps).^2*rho0*S;
  [T1 rpm1 pitch1 efficiency1]=hamprop(P0/hp_to_ftlbfps,vc(tt)*mph_to_kts,n0*60,blades,d,AF,pitchlim,"thrust");
  T(tt)=T1;
  efficiency(tt)=efficiency1;
  pitch(tt)=pitch1;
  if tt<length(t);
    vc(tt+1)=vc(tt)+(t(tt+1)-t(tt))*(T(tt)-D(tt)-rollfriction*W)/W*g*mps_to_mph;
    dist(tt+1)=dist(tt)+(t(tt+1)-t(tt))*vc(tt)*mile_to_ft/3600;
  endif;
endfor;
[etmax idx]=min(abs(vc-vrot));
[etmax idx2]=min(abs(vc-VXManual));
printf("VROT: Distance=%.fft Time=%.2fs\n",dist(idx),t(idx));
printf("VX: Distance=%.fft Time=%.2fs\n",dist(idx2),t(idx2));
clf;
[ax h1 h2]=plotyy(t,vc,t,dist,@plot,@plot);
title("Sea Level Ground Run");
xlabel("Time [s]");
ylabel(ax(1), "Velocity [MPH]");
ylabel(ax(2), "Distance [ft]");
set(h1,"displayname","Velocity");
set(h2,"displayname","Distance");
grid on;
line([t(idx) t(idx)],[0 1200],"displayname",sprintf("Dist=%.fft Time=%.2fs",dist(idx),t(idx)),"color",[1 0 1]);
line([t(idx2) t(idx2)],[0 1200],"displayname",sprintf("Dist=%.fft Time=%.2fs",dist(idx2),t(idx2)),"color",[1 0 1]);
set(ax(1),"ylim",[0 100]);
set(ax(2),"ylim",[0 1200]);
set(ax(1),"ycolor",[0 0 0]);
set(ax(2),"ycolor",[0 0 0]);
legend(ax(1), "location","northwest");
legend(ax(1), "show");
print("p28rslgndrun.eps","-depsc2","-portrait","-color");

clf;
[ax h1 h2]=plotyy(t,pitch,t,efficiency*100,@plot,@plot);
title("Sea Level Ground Run Propeller Parameters");
xlabel("Time [s]");
ylabel(ax(1), "Pitch [degrees]");
ylabel(ax(2), "Efficiency [%]");
set(h1,"displayname","Velocity");
set(h2,"displayname","Distance");
grid on;
line([t(idx) t(idx)],[0 100],"displayname",sprintf("Dist=%.fft Time=%.2fs",dist(idx),t(idx)),"color",[1 0 1]);
line([t(idx2) t(idx2)],[0 100],"displayname",sprintf("Dist=%.fft Time=%.2fs",dist(idx2),t(idx2)),"color",[1 0 1]);
set(ax(1),"ylim",[14 20]);
set(ax(2),"ylim",[0 80]);
set(ax(1),"ycolor",[0 0 0]);
set(ax(2),"ycolor",[0 0 0]);
legend(ax(1), "location","northwest");
legend(ax(1), "show");
print("p28rslgndrunprop.eps","-depsc2","-portrait","-color");

CD0s=[CD0 CD0geardown];
CD0txt=cell(4,length(CD0s));
CD0txt{1,1}="Best Rate of Climb Speed";
CD0txt{2,1}="p28rclimbforce.eps";
CD0txt{3,1}="Climb Propeller Efficiency / Blade Pitch";
CD0txt{4,1}="p28rclimbprop.eps";
CD0txt{1,2}="Best Rate of Climb Speed, Gear Down";
CD0txt{2,2}="p28rclimbforcegd.eps";
CD0txt{3,2}="Climb Propeller Efficiency / Blade Pitch, Gear Down";
CD0txt{4,2}="p28rclimbpropgd.eps";
for CD0i=1:length(CD0s);
  CD0x=CD0s(CD0i);
  vc=(50:0.1:160);
  vcftps=vc*mph_to_ftps;
  vckts=vc*mph_to_kts;
  D=0.5*CD0x*vcftps.^2*rho0*S+W^2./(0.5*vcftps.^2*rho0*S*pi*A*ee);
  [T rpm pitch efficiency]=hamprop(P0/hp_to_ftlbfps,[vckts],n0*60,blades,d,AF,pitchlim,"thrust");
  [p t rho]=icaoatmo([0 10000]);
  D1=0.5*CD0x*vcftps.^2*rho0*S+W^2./(0.5*vcftps.^2*rho0*S*pi*A*ee);
  [T1 rpm1 pitch1 efficiency1]=hamprop(P0*(rho(2)/rho(1)-C)/(1-C)/hp_to_ftlbfps*rho(1)/rho(2),[vckts*sqrt(rho(1)/rho(2))],n0*60,blades,d,AF,pitchlim,"thrust");
  T1=T1.*rho(2)/rho(1);
  clf;
  col=jet(5);
  plot(vc,T,";Thrust;","color",col(1,:));
  hold on;
  plot(vc,D,";Drag;","color",col(2,:));
  plot(vc,T-D,";Excess Thrust;","color",col(3,:));
  hold off;
  title(CD0txt{1,CD0i});
  xlabel("CAS [MPH]");
  ylabel("Force [lbf]");
  grid on;
  [etmax idx]=max(T-D);
  VVCX=vc(idx);
  [etmax idx]=max(vc.*(T-D));
  VVCY=vc(idx);
  [etmax idx]=max(vc.*(T1-D1));
  VVCYFL100=vc(idx);
  if CD0i==1;
     VCX=VVCX;
     VCY=VVCY;
     VCYFL100=VVCYFL100;
  else
     VCXGD=VVCX;
     VCYGD=VVCY;
     VCYGDFL100=VVCYFL100;
  endif;
  line([VVCX VVCX],[0 800],"displayname",sprintf("VX=%5.1fMPH",VVCX),"color",col(4,:));
  line([VVCY VVCY],[0 800],"displayname",sprintf("VY=%5.1fMPH",VVCY),"color",col(5,:));
  line([VVCYFL100 VVCYFL100],[0 800],"displayname",sprintf("VY(FL100)=%5.1fMPH",VVCYFL100),"color",col(5,:));
  printf("VX=%5.1fMPH VY=%5.1fMPH VY(FL100)=%5.1fMPH\n",VCX,VCY,VCYFL100);
  print(CD0txt{2,CD0i},"-depsc2","-portrait","-color");
  clf;
  ax=plotyy(vc,efficiency*100,vc,pitch,@plot,@plot);
  title(CD0txt{3,CD0i});
  xlabel("CAS [MPH]");
  ylabel(ax(1), "Efficiency [%]");
  ylabel(ax(2), "Blade Pitch [degrees]");
  grid on;
  line([VVCX VVCX],[0 100],"displayname",sprintf("VX=%5.1fMPH",VVCX),"color",col(4,:));
  line([VVCY VVCY],[0 100],"displayname",sprintf("VY=%5.1fMPH",VVCY),"color",col(5,:));
  line([VCYFL100 VCYFL100],[0 100],"displayname",sprintf("VY(FL100)=%5.1fMPH",VVCYFL100),"color",col(5,:));
  set(ax(1),"ylim",[0 100]);
  set(ax(2),"ylim",[14 24]);
  set(ax(1),"ycolor",[0 0 0]);
  set(ax(2),"ycolor",[0 0 0]);
  print(CD0txt{4,CD0i},"-depsc2","-portrait","-color");
endfor;

VC2=95;
#VC3=85;
VC3=100;
alt=0:100:18000;
[p t rho]=icaoatmo(alt);
rpms=[2700 2500 2400];
for rpmi=1:length(rpms);
  n=rpms(rpmi)/60;
  PWR=P0*(n/n0)^CRPM*(rho/rho(1)-C)/(1-C);
  D=0.5*CD0*VCY^2*mph_to_ftps^2*rho0*S+W^2./(0.5*VCY^2*mph_to_ftps^2*rho0*S*pi*A*ee);
  [T rpm pitch efficiency]=hamprop(PWR/hp_to_ftlbfps*rho(1)./rho,VCY*mph_to_kts*sqrt(rho(1)./rho),n*60,blades,d,AF,pitchlim,"thrust");
  T=T.*rho/rho(1);
  ROC=VCY*mph_to_kts*nmi_to_ft/60*sqrt(rho(1)./rho)/W.*(T-D);
  D2=0.5*CD0*VC2^2*mph_to_ftps^2*rho0*S+W^2./(0.5*VC2^2*mph_to_ftps^2*rho0*S*pi*A*ee);
  [T2 rpm2 pitch2 efficiency2]=hamprop(PWR/hp_to_ftlbfps*rho(1)./rho,VC2*mph_to_kts*sqrt(rho(1)./rho),n*60,blades,d,AF,pitchlim,"thrust");
  T2=T2.*rho/rho(1);
  ROC2=VC2*mph_to_kts*nmi_to_ft/60*sqrt(rho(1)./rho)/W.*(T2-D2);
  D3=0.5*CD0*VC3^2*mph_to_ftps^2*rho0*S+W^2./(0.5*VC3^2*mph_to_ftps^2*rho0*S*pi*A*ee);
  [T3 rpm3 pitch3 efficiency3]=hamprop(PWR/hp_to_ftlbfps*rho(1)./rho,VC3*mph_to_kts*sqrt(rho(1)./rho),n*60,blades,d,AF,pitchlim,"thrust");
  T3=T3.*rho/rho(1);
  ROC3=VC3*mph_to_kts*nmi_to_ft/60*sqrt(rho(1)./rho)/W.*(T3-D3);
  clf;
  plot(ROC,alt,sprintf(";VCAS=%5.1fMPH;",VCY),ROC2,alt,sprintf(";VCAS=%5.1fMPH;",VC2),ROC3,alt,sprintf(";VCAS=%5.1fMPH;",VC3));
  title(sprintf("Rate of Climb, RPM=%u",n*60));
  xlabel("ROC [ft/min]");
  ylabel("Altitude [ft]");
  grid on;
  axis([0 1200 0 max(alt)]);
  print(sprintf("p28rroc%u.eps",n*60),"-depsc2","-portrait","-color");
  clf;
  plot(efficiency*100,alt,sprintf(";VCAS=%5.1fMPH;",VCY),efficiency2*100,alt,sprintf(";VCAS=%5.1fMPH;",VC2),efficiency3*100,alt,sprintf(";VCAS=%5.1fMPH;",VC3));
  title(sprintf("Climb Propeller Efficiency, RPM=%u",n*60));
  xlabel("Propeller Efficiency [%]");
  ylabel("Altitude [ft]");
  grid on;
  axis([70 100 0 max(alt)]);
  legend("location","northwest");
  print(sprintf("p28rroceff%u.eps",n*60),"-depsc2","-portrait","-color");
  clf;
  plot(pitch,alt,sprintf(";VCAS=%5.1fMPH;",VCY),pitch2,alt,sprintf(";VCAS=%5.1fMPH;",VC2),pitch3,alt,sprintf(";VCAS=%5.1fMPH;",VC3));
  title(sprintf("Climb Propeller Pitch, RPM=%u",n*60));
  xlabel("Propeller Pitch [degrees]");
  ylabel("Altitude [ft]");
  grid on;
  axis([19 24 0 max(alt)]);
  legend("location","northwest");
  print(sprintf("p28rrocpitch%u.eps",n*60),"-depsc2","-portrait","-color");
endfor;

CD0s=[CD0 CD0geardown];
VCYs=[VCYFL100 VCYGDFL100];
CD0txt=cell(6,length(CD0s));
CD0txt{1,1}="Cruise Speed";
CD0txt{2,1}="p28rcruisespeed.eps";
CD0txt{3,1}="Propeller Efficiency";
CD0txt{4,1}="p28rcruiseeff.eps";
CD0txt{5,1}="Propeller Pitch";
CD0txt{6,1}="p28rcruisepitch.eps";
CD0txt{1,2}="Cruise Speed, Gear Down";
CD0txt{2,2}="p28rcruisespeedgd.eps";
CD0txt{3,2}="Propeller Efficiency, Gear Down";
CD0txt{4,2}="p28rcruiseeffgd.eps";
CD0txt{5,2}="Propeller Pitch, Gear Down";
CD0txt{6,2}="p28rcruisepitchgd.eps";
for CD0i=1:length(CD0s);
  CD0x=CD0s(CD0i);
  VVCY=VCYs(CD0i);
  clf;
  relpwrs=0.55:0.1:0.75;
  ftrpms=[2400 2700];
  cols=jet(length(ftrpms)+length(relpwrs));
  pitches=cell(length(ftrpms)+length(relpwrs),1);
  efficiencies=cell(length(ftrpms)+length(relpwrs),1);
  for relpwri=1:length(relpwrs);
    relpwr=relpwrs(relpwri);
    n=2400/60;
    PWR=P0*relpwr;
    alt=0:100:17000;
    [p t rho]=icaoatmo(alt);
    alt=alt(1:max((1:length(alt)).*((rho/rho(1)-C)/(1-C)*n/n0>=relpwr)));
    p=p(1:length(alt));
    t=t(1:length(alt));
    rho=rho(1:length(alt));
    # check solvability
    D=0.5*CD0x*VVCY.^2*mph_to_ftps^2*rho0.*rho/rho(1)*S+W^2./(0.5*VVCY.^2*mph_to_ftps^2*rho0.*rho/rho(1)*S*pi*A*ee);
    [T rpm pitch efficiency]=hamprop(PWR/hp_to_ftlbfps*rho(1)./rho,VVCY*mph_to_kts,n*60,blades,d,AF,pitchlim,"thrust");
    T=T.*rho/rho(1);
    alt=alt(1:max((1:length(alt)).*(T>=D)));
    if length(alt)>0;
      p=p(1:length(alt));
      t=t(1:length(alt));
      rho=rho(1:length(alt));
      # initial guess
      vtcruise=ones(size(alt,1),size(alt,2))*2*VVCY;
      for t=0:39;
	D=0.5*CD0x*vtcruise.^2*mph_to_ftps^2*rho0.*rho/rho(1)*S+W^2./(0.5*vtcruise.^2*mph_to_ftps^2*rho0.*rho/rho(1)*S*pi*A*ee);
	[T rpm pitch efficiency]=hamprop(PWR/hp_to_ftlbfps*rho(1)./rho,[vtcruise*mph_to_kts],n*60,blades,d,AF,pitchlim,"thrust");
	T=T.*rho/rho(1);
	D1=0.5*CD0x*(vtcruise+1).^2*mph_to_ftps^2*rho0.*rho/rho(1)*S+W^2./(0.5*(vtcruise+1).^2*mph_to_ftps^2*rho0.*rho/rho(1)*S*pi*A*ee);
	[T1 rpm1 pitch1 efficiency1]=hamprop(PWR/hp_to_ftlbfps*rho(1)./rho,[(vtcruise+1)*mph_to_kts],n*60,blades,d,AF,pitchlim,"thrust");
	T1=T1.*rho/rho(1);
	e=T-D;
	de=T1-D1-e;
	vtcruise=min(max(vtcruise-e./de,20),200);
      endfor;
      [T rpm pitch efficiency]=hamprop(PWR/hp_to_ftlbfps*rho(1)./rho,[vtcruise*mph_to_kts],n*60,blades,d,AF,pitchlim,"thrust");
      pitches{length(ftrpms)+relpwri}=pitch;
      efficiencies{length(ftrpms)+relpwri}=efficiency;
      plot(vtcruise,alt,sprintf(";%2.0f%%, RPM=%.0f;",relpwr*100,n*60),"color",cols(length(ftrpms)+relpwri,:));
      hold on;
    endif;
  endfor;
  for tt=1:length(ftrpms);
    alt=0:100:17000;
    [p t rho]=icaoatmo(alt);
    n=ftrpms(tt)/60;
    PWR=P0*(rho/rho(1)-C)/(1-C)*n/n0;
    # check solvability
    D=0.5*CD0x*VVCY.^2*mph_to_ftps^2*rho0.*rho/rho(1)*S+W^2./(0.5*VVCY.^2*mph_to_ftps^2*rho0.*rho/rho(1)*S*pi*A*ee);
    [T rpm pitch efficiency]=hamprop(PWR/hp_to_ftlbfps*rho(1)./rho,VVCY*mph_to_kts,n*60,blades,d,AF,pitchlim,"thrust");
    T=T.*rho/rho(1);
    alt=alt(1:max((1:length(alt)).*(T>=D)));
    p=p(1:length(alt));
    t=t(1:length(alt));
    rho=rho(1:length(alt));
    PWR=PWR(1:length(alt));
    # initial guess
    vtcruise=ones(size(alt,1),size(alt,2))*2*VVCY;
    for t=0:39;
      D=0.5*CD0x*vtcruise.^2*mph_to_ftps^2*rho0.*rho/rho(1)*S+W^2./(0.5*vtcruise.^2*mph_to_ftps^2*rho0.*rho/rho(1)*S*pi*A*ee);
      [T rpm pitch efficiency]=hamprop(PWR/hp_to_ftlbfps*rho(1)./rho,[vtcruise*mph_to_kts],n*60,blades,d,AF,pitchlim,"thrust");
      T=T.*rho/rho(1);
      D1=0.5*CD0x*(vtcruise+1).^2*mph_to_ftps^2*rho0.*rho/rho(1)*S+W^2./(0.5*(vtcruise+1).^2*mph_to_ftps^2*rho0.*rho/rho(1)*S*pi*A*ee);
      [T1 rpm1 pitch1 efficiency1]=hamprop(PWR/hp_to_ftlbfps*rho(1)./rho,[(vtcruise+1)*mph_to_kts],n*60,blades,d,AF,pitchlim,"thrust");
      T1=T1.*rho/rho(1);
      e=T-D;
      de=T1-D1-e;
      vtcruise=min(max(vtcruise-e./de,20),200);
    endfor;
    [T rpm pitch efficiency]=hamprop(PWR/hp_to_ftlbfps*rho(1)./rho,[vtcruise*mph_to_kts],n*60,blades,d,AF,pitchlim,"thrust");
    pitches{tt}=pitch;
    efficiencies{tt}=efficiency;
    plot(vtcruise,alt,sprintf(";Full Throttle, RPM=%.f;",n*60),"color",cols(tt,:));
    hold on;
  endfor;
  hold off;
  title(CD0txt{1,CD0i});
  xlabel("Cruise Speed (TAS) [MPH]");
  ylabel("Altitude [ft]");
  grid on;
  print(CD0txt{2,CD0i},"-depsc2","-portrait","-color");

  alt=0:100:17000;
  cols=jet(length(ftrpms)+length(relpwrs));
  clf;
  for t=1:length(ftrpms);
    plot(efficiencies{t}*100,alt(1:size(efficiencies{t},2)),sprintf(";Full Throttle, RPM=%.f;",ftrpms(t)),"color",cols(t,:));
    hold on;
  endfor;
  for t=1:length(relpwrs);
    plot(efficiencies{length(ftrpms)+t}*100,alt(1:size(efficiencies{length(ftrpms)+t},2)),sprintf(";%2.0f%%, RPM=%.0f;",relpwrs(t)*100,2400),"color",cols(length(ftrpms)+t,:));
  endfor;
  title(CD0txt{3,CD0i});
  xlabel("Efficiency [%]");
  ylabel("Altitude [ft]");
  grid on;
  print(CD0txt{4,CD0i},"-depsc2","-portrait","-color");

  clf;
  for t=1:length(ftrpms);
    plot(pitches{t},alt(1:size(pitches{t},2)),sprintf(";Full Throttle, RPM=%.f;",ftrpms(t)),"color",cols(t,:));
    hold on;
  endfor;
  for t=1:length(relpwrs);
    plot(pitches{length(ftrpms)+t},alt(1:size(pitches{length(ftrpms)+t},2)),sprintf(";%2.0f%%, RPM=%.0f;",relpwrs(t)*100,2400),"color",cols(length(ftrpms)+t,:));
  endfor;
  title(CD0txt{5,CD0i});
  xlabel("Pitch [degrees]");
  ylabel("Altitude [ft]");
  grid on;
  print(CD0txt{6,CD0i},"-depsc2","-portrait","-color");
endfor;

for plimidx=1:2;
  vc=(50:0.1:160);
  vcftps=vc*mph_to_ftps;
  vckts=vc*mph_to_kts;
  D=0.5*CD0*vcftps.^2*rho0*S+W^2./(0.5*vcftps.^2*rho0*S*pi*A*ee);
  [T rpm pitch efficiency]=hamprop(0,[vckts],n0*60,blades,d,AF,pitchlim(plimidx),"thrust");
  [p t rho]=icaoatmo([0 10000]);
  D1=0.5*CD0*vcftps.^2*rho0*S+W^2./(0.5*vcftps.^2*rho0*S*pi*A*ee);
  [T1 rpm1 pitch1 efficiency1]=hamprop(0,[vckts*sqrt(rho(1)/rho(2))],n0*60,blades,d,AF,pitchlim(plimidx),"thrust");
  T1=T1.*rho(2)/rho(1);
  clf;
  col=jet(5);
  plot(vc,T,";Thrust;","color",col(1,:));
  hold on;
  plot(vc,D,";Drag;","color",col(2,:));
  plot(vc,T-D,";Excess Thrust;","color",col(3,:));
  hold off;
  title(sprintf("Best Glide Speed, Prop Pitch %.f",pitchlim(plimidx)));
  xlabel("CAS [MPH]");
  ylabel("Force [lbf]");
  grid on;
  [etmax idx]=min(D);
  Vbestglide=vc(idx)
  [etmax idx]=max(1./(D-T));
  Vbestglide1=vc(idx)
  line([Vbestglide Vbestglide],[-600 600],"displayname",sprintf("Vbestglide=%5.1fMPH",Vbestglide),"color",col(4,:));
  line([Vbestglide1 Vbestglide1],[-600 600],"displayname",sprintf("Vbestglide1=%5.1fMPH",Vbestglide1),"color",col(5,:));
  printf("Vbestglide=%5.1fMPH Vbestglide1=%5.1fMPH\n",Vbestglide,Vbestglide1);
  print(sprintf("p28rbestglidepp%u.eps",round(pitchlim(plimidx))),"-depsc2","-portrait","-color");
  clf;
  plot(vc,rpm);
  title(sprintf("Glide Propeller Windmilling RPM, Prop Pitch %.f",pitchlim(plimidx)));
  xlabel("CAS [MPH]");
  ylabel("RPM [min^{-1}]");
  grid on;
  tt=[100*floor(min(rpm)/100) 100*ceil(max(rpm)/100)];
  line([Vbestglide Vbestglide],tt,"displayname",sprintf("Vbestglide=%5.1fMPH",Vbestglide),"color",col(4,:));
  line([Vbestglide1 Vbestglide1],tt,"displayname",sprintf("Vbestglide1=%5.1fMPH",Vbestglide1),"color",col(5,:));
  axis([min(vc) max(vc) tt]);
  print(sprintf("p28rbestglideproppp%u.eps",round(pitchlim(plimidx))),"-depsc2","-portrait","-color");

  alt=0:100:18000;
  [p t rho]=icaoatmo(alt);
  vc=[Vbestglide Vbestglide1];
  ROD=zeros(length(vc),size(alt,2));
  rpm=zeros(length(vc),size(alt,2));
  dist=zeros(length(vc),size(alt,2));
  clf;
  for vci=1:length(vc);
    D=0.5*CD0*vc(vci)^2*mph_to_ftps^2*rho0*S+W^2./(0.5*vc(vci)^2*mph_to_ftps^2*rho0*S*pi*A*ee);
    [T rpm1 pitch efficiency]=hamprop(0,vc(vci)*mph_to_kts*sqrt(rho(1)./rho),n0*60,blades,d,AF,pitchlim(plimidx),"thrust");
    T=T.*rho/rho(1);
    rpm(vci,:)=rpm1;
    ROD(vci,:)=vc(vci)*mph_to_kts*nmi_to_ft/60*sqrt(rho(1)./rho)/W.*(D-T);
    #dist(vci,2:length(alt))=cumsum((alt(2:length(alt))-alt(1:length(alt)-1))./ROD(vci,1:length(alt)-1)/60*vc(vci).*sqrt(rho(1)./rho(1:length(alt)-1)));
    dist(vci,2:length(alt))=cumsum((alt(2:length(alt))-alt(1:length(alt)-1))*W./(D-T)(1:length(alt)-1))/(mph_to_kts*nmi_to_ft);
    plot(ROD(vci,:),alt,sprintf("%u;VCAS=%5.1fMPH;",vci*2-1,vc(vci)));
    hold on;
  endfor;
  hold off;
  title(sprintf("Glide: Rate of Descent, Prop Pitch %.f",pitchlim(plimidx)));
  xlabel("ROD [ft/min]");
  ylabel("Altitude [ft]");
  grid on;
  axis([100*floor(min(min(ROD))/100) 100*ceil(max(max(ROD))/100) 0 max(alt)]);
  legend("location","northwest");
  print(sprintf("p28rgliderodpp%u.eps",round(pitchlim(plimidx))),"-depsc2","-portrait","-color");

  clf;
  for vci=1:length(vc);
    plot(dist(vci,:),alt,sprintf("%u;VCAS=%5.1fMPH;",vci*2-1,vc(vci)));
    hold on;
  endfor;
  hold off;
  title(sprintf("Glide: Distance, Prop Pitch %.f",pitchlim(plimidx)));
  xlabel("Distance [miles]");
  ylabel("Altitude [ft]");
  grid on;
  axis([0 5*ceil(max(max(dist))/5) 0 max(alt)]);
  legend("location","northwest");
  print(sprintf("p28rglidedistpp%u.eps",round(pitchlim(plimidx))),"-depsc2","-portrait","-color");

  clf;
  for vci=1:length(vc);
    plot(rpm(vci,:),alt,sprintf("%u;VCAS=%5.1fMPH;",vci*2-1,vc(vci)));
    hold on;
  endfor;
  hold off;
  title(sprintf("Glide: Windmilling RPM, Prop Pitch %.f",pitchlim(plimidx)));
  xlabel("RPM [min^{-1}]");
  ylabel("Altitude [ft]");
  grid on;
  axis([100*floor(min(min(rpm))/100) 100*ceil(max(max(rpm))/100) 0 max(alt)]);
  legend("location","northwest");
  print(sprintf("p28rgliderpmpp%u.eps",round(pitchlim(plimidx))),"-depsc2","-portrait","-color");
endfor;
