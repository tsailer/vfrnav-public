#!/usr/bin/octave

graphics_toolkit ("gnuplot");

# diam 192

t = [ 0 5.5 ;
      1 6.2 ;
      2 7 ;
      3 7.4 ;
      4 8 ;
      5 8.4 ;
      6 9 ;
      7 9.3 ;
      8 9.8 ;
      9 10.4 ;
      10 10.6 ;
      11 11.2 ;
      12 11.6 ;
      13 11.9 ;
      14 12.2 ;
      15 12.4 ;
      16 12.9 ;
      17 13 ;
      18 13.4 ;
      19 13.7 ;
      20 14 ;
      21 14 ;
      22 14.3 ;
      23 14.3 ;
      24 14.7 ;
      25 14.8 ;
      26 15 ;
      27 14.9 ;
      28 15.2 ;
      29 15.1 ;
      30 15.4 ;
      31 15.4 ;
      32 15.6 ;
      33 15.5 ;
      34 15.7 ;
      35 15.6 ;
      36 15.9 ;
      37 16 ;
      38 16 ;
      39 16 ;
      40 16.2 ;
      41 16 ;
      42 16.2 ;
      43 16.1 ;
      44 16.1 ;
      45 16.1 ;
      46 16.1 ;
      47 16 ;
      48 16 ;
      49 16 ;
      50 16 ;
      51 16 ;
      52 16 ;
      53 15.9 ;
      54 15.9 ;
      55 15.9 ;
      56 15.8 ;
      57 15.8 ;
      58 15.4 ;
      59 15.5 ;
      60 15 ;
      61 15 ;
      62 14.8 ;
      63 14.7 ;
      64 14.4 ;
      65 14.6 ;
      66 14.2 ;
      67 14.3 ;
      68 13.8 ;
      69 13.3 ;
      70 13.4 ;
      71 13.1 ;
      72 12.8 ;
      73 12.3 ;
      74 12.2 ;
      75 11.7 ;
      76 11.4 ;
      77 11.3 ;
      78 11 ;
      79 10.3 ];


# vander (vandermonde matrix)
rmax=74/2;
rmin=rmax-max(t(:,1))/2.54;
rcowling=88/2/2.54;
polyorder=4;
P = polyfit(74/2-t(:,1)/2.54,t(:,2)/2.54,polyorder);

r=((round(rmin*1e2)*1e-2):1e-2:rmax);
w=polyval(P,r);
figure("visible","off");
plot(r,w,";Approximation;",...
     74/2-t(:,1)/2.54,t(:,2)/2.54,"+;Measurements;");
line([rcowling rcowling],[0 10],"displayname","Cowling","color","cyan");
axis([floor(rmin) ceil(rmax) 0 ceil(max(w))]);
grid("on");
title("Hartzell F7666A-2");
xlabel("Radius [inch]");
ylabel("Width [inch]");

Pint=polyint(P);
AF=polyval(Pint,rmax)-polyval(Pint,rmin);
AFwoc=polyval(Pint,rmax)-polyval(Pint,rcowling);

text(10.1, 1.4, sprintf("AF=%.1f",AF));
text(10.1, 1.2, sprintf("AF=%.1f (without cowling)",AFwoc));

legend("location","northeast");
print("hartzellf7666a2af.eps","-depsc2","-portrait","-color");
