#!/usr/bin/python
# -*- coding: utf-8 -*-
'''This is a downloader application for weather charts
from flugwetter.de and other sources.

This is free software, released under GPL.
Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, 2013
'''

import urllib2
import datetime
import calendar
import string
import time
import math
import sys
import os
from gi.repository import GObject, GLib, Gtk, Gdk

# global auth handler

PASSWD_FLUGWETTER = 1
PASSWD_FLUGWETTERIMGCOMPR = 2

passwd = 0

password_mgr = urllib2.HTTPPasswordMgrWithDefaultRealm()
try:
    f = open(os.path.expanduser("~/.vfrnav/wetterdl"), 'r')
    for line in f:
        recs = line.split(':', 2)
        if len(recs) < 3:
            continue
        recs[2] = recs[2].rstrip()
        password_mgr.add_password(realm=None,uri=recs[2],user=recs[0],passwd=recs[1])
        if recs[2] == 'http://www.flugwetter.de/':
            passwd |= PASSWD_FLUGWETTER
        if recs[2] == 'http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php':
            passwd |= PASSWD_FLUGWETTERIMGCOMPR
    f.close()
except Exception:
    sys.exc_clear()
auth_handler = urllib2.HTTPBasicAuthHandler(password_mgr)
opener = urllib2.build_opener(auth_handler)
urllib2.install_opener(opener)
try:
    os.mkdir(os.path.expanduser("~/.vfrnav"))
except Exception:
    sys.exc_clear()
try:
    os.mkdir(os.path.expanduser("~/.vfrnav/bitmapmaps"))
except Exception:
    sys.exc_clear()
try:
    os.mkdir(os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl"))
except Exception:
    sys.exc_clear()

class DownloadWindow(Gtk.Window):

    def __init__(self):
        Gtk.Window.__init__(self, title="Flugwetter Downloader")
        self.set_border_width(10)
        self.set_resizable(True)

        vbox = Gtk.VBox(spacing=6)
        self.add(vbox)

        self.queuelabel = Gtk.Label()
        vbox.pack_start(self.queuelabel, False, False, 5)

        self.queueprogress = Gtk.ProgressBar()
        vbox.pack_start(self.queueprogress, False, False, 5)

        separator = Gtk.HSeparator()
        vbox.pack_start(separator, False, False, 0)

        self.filelabel = Gtk.Label()
        vbox.pack_start(self.filelabel, False, False, 5)

        self.fileprogress = Gtk.ProgressBar()
        vbox.pack_start(self.fileprogress, False, False, 5)

        separator = Gtk.HSeparator()
        vbox.pack_start(separator, False, False, 0)

        vpaned = Gtk.VPaned()
        vbox.pack_start(vpaned, True, True, 0)

        sw = Gtk.ScrolledWindow()
        sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        self.image = Gtk.Image()
        sw.add_with_viewport(self.image)
        vpaned.pack1(sw, True, True)

        self.store = Gtk.ListStore(str,str,int)
        self.tree = Gtk.TreeView(self.store)
        sw = Gtk.ScrolledWindow()
        sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        sw.add_with_viewport(self.tree)
        renderer = Gtk.CellRendererText()
        column = Gtk.TreeViewColumn("Chart", renderer, text=0)
        column.set_sort_column_id(0)
        column.set_resizable(True)
        column.set_expand(True)
        self.tree.append_column(column)
        renderer = Gtk.CellRendererText()
        renderer.set_property('xalign', 1)
        column = Gtk.TreeViewColumn("Size", renderer, text=2)
        column.set_sort_column_id(2)
        column.set_resizable(True)
        column.set_expand(False)
        self.tree.append_column(column)
        select = self.tree.get_selection()
        select.connect("changed", self.on_tree_selection_changed)
        select.set_mode(Gtk.SelectionMode.BROWSE)
        vpaned.pack2(sw, False, True)

        separator = Gtk.HSeparator()
        vbox.pack_start(separator, False, False, 0)

        buttonquit = Gtk.Button(stock=Gtk.STOCK_QUIT)
        buttonquit.connect("clicked", self.on_quit_clicked)
        vbox.pack_start(buttonquit, False, False, 0)

        self.nrfiles = 0
        self.dlqueue = []
        self.curfilenr = -1
        self.curfiletmo = -1
        self.curfilesize = 0
        self.curfilecnt = 0
        self.curtime = 0
        self.url = ""
        self.locfile = ""
        self.xmlfile = ""

        self.updatestat()

    def on_quit_clicked(self, button):
        Gtk.main_quit()

    def addqueue(self, q):
        self.dlqueue[len(self.dlqueue):] = q
        if self.curfilenr >= 0:
            return
        while 1:
            if len(self.dlqueue) < 1:
                break
            self.curfilenr = self.dlqueue[0][5]
            if self.curfilenr <= 0:
                self.dlqueue[0:1] = []
                continue
            self.curfilecnt = 0
            timeint = self.dlqueue[0][3]
            timeoffs = self.dlqueue[0][4]
            self.curtime = datetime.datetime.utcfromtimestamp((int(time.time()-timeoffs)/timeint+1)*timeint+timeoffs)
            self.startfile()
            break
        self.updatestat()

    def updatestat(self):
        nrfilestogo = 0
        for r in self.dlqueue[:]:
            nrfilestogo += int(r[5])
        totnrfiles = self.nrfiles + nrfilestogo
        if totnrfiles <= 0:
            self.queueprogress.set_fraction(1)
            self.queueprogress.set_text("")
            self.queuelabel.set_text("")
        else:
            self.queueprogress.set_fraction(self.nrfiles / totnrfiles)
            self.queueprogress.set_text("{0} / {1}".format(self.nrfiles, totnrfiles))
            self.queuelabel.set_text("{0} / {1}".format(self.nrfiles, totnrfiles))
        if self.curfilenr < 0 or self.curfilesize <= 0:
            self.fileprogress.set_fraction(1)
            self.fileprogress.set_text("")
            self.filelabel.set_text("")
        else:
            self.fileprogress.set_fraction(self.curfilecnt / self.curfilesize)
            self.fileprogress.set_text("{0} ({1} / {2})".format(self.locfile, self.curfilecnt, self.curfilesize))
            self.filelabel.set_text("{0} ({1} / {2})".format(self.locfile, self.curfilecnt, self.curfilesize))

    def startfile(self):
        if self.curfilenr <= 0:
            self.dlqueue[0:1] = []
            self.curfilenr = -1
            self.curfiletmo = -1
            self.addqueue([])
            return
        while 1:
            curtimestr = self.curtime.strftime(self.dlqueue[0][2])
            blitzortungtdiff = self.curtime - datetime.datetime(1970, 1, 1, 0, 0, 0, 0)
            blitzortungt = int(blitzortungtdiff.days * (24*60) + math.ceil(blitzortungtdiff.seconds / 60))+2
            self.url = self.dlqueue[0][0].format(curtimestr, blitzortungt)
            self.locfile = self.dlqueue[0][1].format(curtimestr)
            self.xmlfile = self.dlqueue[0][7].format(curtimestr)
            self.updatestat()
            if os.path.exists(self.locfile):
                self.curfilenr -= 1
                self.curtime -= datetime.timedelta(seconds=self.dlqueue[0][3])
                self.startfile()
                return
            try:
                print "opening URL:", self.url
                self.fin = urllib2.urlopen(self.url)
                cl = self.fin.info().getheaders("Content-Length")
                #print "content length:", cl, "headers:", self.fin.info()
                self.curfilesize = self.dlqueue[0][6] * 1024
                if len(cl) >= 1:
                    self.curfilesize = int(cl[0])
                if self.curfilesize >= 1024:
                    self.curfilecnt = 0
                    self.fout = open(self.locfile, 'w')
                    self.updatestat()
                    GObject.io_add_watch(self.fin, GObject.IO_IN | GObject.IO_PRI | GObject.IO_HUP | GObject.IO_ERR,
                                         self.network_io_callback)
                    return

            except urllib2.URLError as e:
                print "URL Error:", e.reason, "URL:", self.url
                self.dlqueue[0:1] = []
                self.curfilenr = -1
                self.curfiletmo = -1
                self.addqueue([])
                return

            except urllib2.HTTPError as e:
                print "HTTP Error:", e.reason, "URL:", self.url
                if e.code() != 404:
                    self.dlqueue[0:1] = []
                    self.curfilenr = -1
                    self.curfiletmo = -1
                    self.addqueue([])
                    return

            self.curfiletmo += 1
            if self.curfiletmo >= 10:
                self.dlqueue[0:1] = []
                self.curfilenr = -1
                self.curfiletmo = -1
                self.addqueue([])
                return

            self.fin.close()
            self.curtime -= datetime.timedelta(seconds=self.dlqueue[0][3])

    def network_io_callback(self, source, condition):
        while Gtk.events_pending():
            Gtk.main_iteration()
        data = self.fin.read(1024)
        #print "IO Callback:", condition, "data len:", len(data)
        if len(data) > 0:
            self.fout.write(data)
            self.curfilecnt += len(data)
            self.updatestat()
        if len(data) == 1024:
            return True
        self.fout.close()
        if self.curfilecnt > 0:
            print "writing XML:", self.xmlfile, "image:", self.locfile
            self.fout = open(self.xmlfile, 'w')  
            self.fout.write(self.dlqueue[0][8].format(self.locfile,
                                                      int(calendar.timegm(self.curtime.utctimetuple())),
                                                      self.curtime.strftime("%Y-%m-%d %H:%M:%S")))
            self.fout.close()
            self.nrfiles += 1
            locfilenopath = string.rsplit(self.locfile, '/', 1)
            locfilenopath = locfilenopath[len(locfilenopath)-1]
            self.store.append([locfilenopath, self.locfile, self.curfilecnt])
        self.curfilenr -= 1
        self.curtime -= datetime.timedelta(seconds=self.dlqueue[0][3])
        self.startfile()
        return False

    def on_tree_selection_changed(self, selection):
        model, rows = selection.get_selected_rows()
        for r in rows[:]:
            it = model.get_iter(r)
            try:
                self.image.set_from_file(self.store[it][1])
            except GObject.GError as e:
                print "Cannot load image:", self.store[it][1], ":", e.message
            except GLib.GError as e:
                print "Cannot load image:", self.store[it][1], ":", e.message
            except Exception:
                sys.exc_clear()
                print "Cannot load image:", self.store[it][1]

class MainWindow(Gtk.Window):

    def __init__(self):
        Gtk.Window.__init__(self, title="Flugwetter Downloader")
        self.set_border_width(10)
        self.set_resizable(True)
        self.connect("delete-event", Gtk.main_quit)

        vbox = Gtk.VBox(spacing=2)
        self.add(vbox)

        self.store = Gtk.ListStore(str,str,str,str,int,int,int,str,str)
        self.store.append(["Blitzortung Europe",
                           "http://images.blitzortung.org/Images/image_b_eu.png?t={1}",
                           os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/blitzortung_eu_{0}.png"),
                           "%y%m%d%H%M",60,0,53,
                           os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/blitzortung_eu_{0}.xml"),
                           "<?xml version=\"1.0\"?>\n"
                           "<MovingMap>\n"
                           "  <map name=\"Blitzortung Europe {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"12\">\n"
                           "    <point lat=\"36.996178\" lon=\"-8.947468\" x=\"159\" y=\"705\">Sagres</point>\n"
                           "    <point lat=\"31.252786\" lon=\"32.316906\" x=\"755\" y=\"800\">Port Fuad</point>\n"
                           "    <point lat=\"59.944072\" lon=\"30.300121\" x=\"727\" y=\"275\">St. Petersburg</point>\n"
                           "    <point lat=\"58.985462\" lon=\"-2.963433\" x=\"246\" y=\"297\">Kirkwall</point>\n"
                           "    <point lat=\"43.007685\" lon=\"9.400486\"  x=\"424\" y=\"602\">N. Corsica</point>\n"
                           "  </map>\n"
                           "</MovingMap>"])
        self.store.append(["Blitzortung Central Europe",
                           "http://images.blitzortung.org/Images/image_b_de.png?t={1}",
                           os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/blitzortung_de_{0}.png"),
                           "%y%m%d%H%M",60,0,41,
                           os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/blitzortung_de_{0}.xml"),
                           "<?xml version=\"1.0\"?>\n"
                           "<MovingMap>\n"
                           "  <map name=\"Blitzortung Central Europe {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10\">\n"
                           "    <point lat=\"46.947919\" lon=\"7.444611\"  x=\"314\" y=\"792\">Bern</point>\n"
                           "    <point lat=\"46.056452\" lon=\"14.508069\" x=\"723\" y=\"866\">Ljubljana</point>\n"
                           "    <point lat=\"48.208173\" lon=\"16.373813\" x=\"832\" y=\"684\">Wien</point>\n"
                           "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"719\" y=\"518\">Prag</point>\n"
                           "    <point lat=\"54.323294\" lon=\"10.122764\" x=\"470\" y=\"118\">Kiel</point>\n"     
                           "    <point lat=\"50.85035\"  lon=\"4.351723\"  x=\"136\" y=\"452\">Bruessel</point>\n"
                           "  </map>\n"
                           "</MovingMap>"])
        self.store.append(["Blitzortung Eastern Europe",
                           "http://images.blitzortung.org/Images/image_b_pl.png?t={1}",
                           os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/blitzortung_pl_{0}.png"),
                           "%y%m%d%H%M",60,0,45,
                           os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/blitzortung_pl_{0}.xml"),
                           "<?xml version=\"1.0\"?>\n"
                           "<MovingMap>\n"
                           "  <map name=\"Blitzortung Eastern Europe {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"20\">\n"
                           "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"204\" y=\"456\">Prag</point>\n"
                           "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"158\" y=\"275\">Berlin</point>\n"
                           "    <point lat=\"46.056452\" lon=\"14.508069\" x=\"208\" y=\"735\">Ljubljana</point>\n"
                           "    <point lat=\"44.432493\" lon=\"26.10388\"  x=\"745\" y=\"842\">Bucharest</point>\n"
                           "    <point lat=\"53.900002\" lon=\"27.566671\" x=\"814\" y=\"164\">Minsk</point>\n"
                           "    <point lat=\"52.229679\" lon=\"21.012232\" x=\"510\" y=\"299\">Warsaw</point>\n"
                           "  </map>\n"
                           "</MovingMap>"])
        self.store.append(["Blitzortung South East Europe",
                           "http://images.blitzortung.org/Images/image_b_gr.png?t={1}",
                           os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/blitzortung_gr_{0}.png"),
                           "%y%m%d%H%M",60,0,41,
                           os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/blitzortung_gr_{0}.xml"),
                           "<?xml version=\"1.0\"?>\n"
                           "<MovingMap>\n"
                           "  <map name=\"Blitzortung South East Europe {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"23\">\n"
                           "    <point lat=\"31.252786\" lon=\"32.316906\" x=\"751\" y=\"808\">Port Fuad</point>\n"
                           "    <point lat=\"38.268098\" lon=\"15.637815\" x=\"133\" y=\"493\">Pantano Piccolo</point>\n"
                           "    <point lat=\"45.502711\" lon=\"13.505539\" x=\"55\"  y=\"135\">Groblje</point>\n"
                           "    <point lat=\"44.578226\" lon=\"33.396034\" x=\"791\" y=\"183\">Sevastopol</point>\n"
                           "    <point lat=\"37.983716\" lon=\"23.72931\"  x=\"428\" y=\"508\">Athens</point>\n"
                           "  </map>\n"
                           "</MovingMap>"])
        self.store.append(["Blitzortung South West Europe",
                           "http://images.blitzortung.org/Images/image_b_fr.png?t={1}",
                           os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/blitzortung_fr_{0}.png"),
                           "%y%m%d%H%M",60,0,50,
                           os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/blitzortung_fr_{0}.xml"),
                           "<?xml version=\"1.0\"?>\n"
                           "<MovingMap>\n"
                           "  <map name=\"Blitzortung South West Europe {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"3\">\n"
                           "    <point lat=\"36.996178\" lon=\"-8.947468\" x=\"88\"  y=\"806\">Sagres</point>\n"
                           "    <point lat=\"38.268098\" lon=\"15.637815\" x=\"796\" y=\"760\">Pantano Piccolo</point>\n"
                           "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"629\" y=\"122\">Hamburg</point>\n"
                           "    <point lat=\"50.053305\" lon=\"-5.680919\" x=\"181\" y=\"285\">Sennen</point>\n"
                           "  </map>\n"
                           "</MovingMap>"])
        self.store.append(["Blitzortung United Kingdom",
                           "http://images.blitzortung.org/Images/image_b_uk.png?t={1}",
                           os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/blitzortung_uk_{0}.png"),
                           "%y%m%d%H%M",60,0,28,
                           os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/blitzortung_uk_{0}.xml"),
                           "<?xml version=\"1.0\"?>\n"
                           "<MovingMap>\n"
                           "  <map name=\"Blitzortung United Kingdom {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"-1\">\n"
                           "    <point lat=\"53.349808\" lon=\"-6.260319\" x=\"367\" y=\"512\">Dublin</point>\n"
                           "    <point lat=\"51.511226\" lon=\"-0.11982\"  x=\"625\" y=\"638\">London</point>\n"
                           "    <point lat=\"48.856612\" lon=\"2.352211\"  x=\"730\" y=\"812\">Paris</point>\n"
                           "    <point lat=\"50.85035\"  lon=\"4.351723\"  x=\"814\" y=\"684\">Bruessel</point>\n"
                           "    <point lat=\"52.370215\" lon=\"4.89516\"   x=\"837\" y=\"581\">Amsterdam</point>\n"
                           "  </map>\n"
                           "</MovingMap>"])
        self.store.append(["Blitzortung Scandinavia",
                           "http://images.blitzortung.org/Images/image_b_sk.png?t={1}",
                           os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/blitzortung_sk_{0}.png"),
                           "%y%m%d%H%M",60,0,48,
                           os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/blitzortung_sk_{0}.xml"),
                           "<?xml version=\"1.0\"?>\n"
                           "<MovingMap>\n"
                           "  <map name=\"Blitzortung Scandinavia {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"20\">\n"
                           "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"259\" y=\"746\">Hamburg</point>\n"
                           "    <point lat=\"69.42542\"  lon=\"28.751929\" x=\"760\" y=\"100\">Lake</point>\n"
                           "    <point lat=\"50.450099\" lon=\"30.52341\"  x=\"805\" y=\"850\">Kiev</point>\n"
                           "  </map>\n"
                           "</MovingMap>"])
        if passwd & PASSWD_FLUGWETTER:
            self.store.append(["Radar Europa",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/pm_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_pm_{0}.png"),
                               "%y%m%d%H%M",300,0,277,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_pm_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Europe {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"5\">\n"
                               "    <point lat=\"45.465458\" lon=\"9.186523\"  x=\"448\" y=\"598\">Mailand</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"519\" y=\"480\">Muenchen</point>\n"
                               "    <point lat=\"48.208173\" lon=\"16.373813\" x=\"660\" y=\"468\">Wien</point>\n"
                               "    <point lat=\"48.856612\" lon=\"2.352211\"  x=\"254\" y=\"434\">Paris</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"437\" y=\"392\">Frankfurt</point>\n"
                               "    <point lat=\"50.85035\"  lon=\"4.351723\"  x=\"320\" y=\"355\">Bruessel</point>\n"
                               "    <point lat=\"52.370215\" lon=\"4.89516\"   x=\"340\" y=\"290\">Amsterdam</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"562\" y=\"288\">Berlin</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"474\" y=\"245\">Hamburg</point>\n"
                               "    <point lat=\"55.676096\" lon=\"12.568338\" x=\"533\" y=\"155\">Kopenhagen</point>\n"
                               "    <point lat=\"51.511226\" lon=\"-0.11982\"  x=\"203\" y=\"310\">London</point>\n"
                               "    <point lat=\"53.349808\" lon=\"-6.260319\" x=\"64\"  y=\"196\">Dublin</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Alpen",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/r5_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_r5_{0}.png"),
                               "%y%m%d%H%M",300,0,1036,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_r5_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Alpen {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10\">\n"
                               "    <point x=\"61\"  y=\"643\" lat=\"43.433584571\" lon=\"5.213687606\">LFML</point>\n"
                               "    <point x=\"225\" y=\"632\" lat=\"43.656067103\" lon=\"7.215508707\">LFMN</point>\n"
                               "    <point x=\"87\"  y=\"426\" lat=\"45.362728536\" lon=\"5.329292323\">LFLS</point>\n"
                               "    <point x=\"132\" y=\"400\" lat=\"45.637590587\" lon=\"5.880322829\">LFLB</point>\n"
                               "    <point x=\"70\"  y=\"384\" lat=\"45.728640854\" lon=\"5.088146813\">LFLL</point>\n"
                               "    <point x=\"270\" y=\"454\" lat=\"45.200763345\" lon=\"7.649630606\">LIMF</point>\n"
                               "    <point x=\"363\" y=\"548\" lat=\"44.411957860\" lon=\"8.841866143\">LIMJ</point>\n"
                               "    <point x=\"357\" y=\"412\" lat=\"45.628117025\" lon=\"8.723268248\">LIMC</point>\n"
                               "    <point x=\"157\" y=\"332\" lat=\"46.238094270\" lon=\"6.109000593\">LSGG</point>\n"
                               "    <point x=\"85\"  y=\"212\" lat=\"47.272201180\" lon=\"5.085252710\">LFSD</point>\n"
                               "    <point x=\"153\" y=\"220\" lat=\"47.274338901\" lon=\"6.084167361\">LFSA</point>\n"
                               "    <point x=\"271\" y=\"187\" lat=\"47.589798868\" lon=\"7.528706230\">LFSB</point>\n"
                               "    <point x=\"266\" y=\"263\" lat=\"46.913436949\" lon=\"7.497960068\">LSZB</point>\n"
                               "    <point x=\"346\" y=\"203\" lat=\"47.457772493\" lon=\"8.553960845\">LSZH</point>\n"
                               "    <point x=\"419\" y=\"186\" lat=\"47.671316564\" lon=\"9.511491694\">EDNY</point>\n"
                               "    <point x=\"401\" y=\"71\"  lat=\"48.689877391\" lon=\"9.221965671\">EDDS</point>\n"
                               "    <point x=\"556\" y=\"618\" lat=\"43.809984326\" lon=\"11.205117591\">LIRQ</point>\n"
                               "    <point x=\"562\" y=\"535\" lat=\"44.535446763\" lon=\"11.288662702\">LIPE</point>\n"
                               "    <point x=\"641\" y=\"423\" lat=\"45.505153835\" lon=\"12.351905778\">LIPZ</point>\n"
                               "    <point x=\"771\" y=\"484\" lat=\"44.893533736\" lon=\"13.922155425\">LDPL</point>\n"
                               "    <point x=\"894\" y=\"565\" lat=\"44.086080194\" lon=\"15.349408463\">LDZD</point>\n"
                               "    <point x=\"933\" y=\"375\" lat=\"45.741893649\" lon=\"16.067201123\">LDZA</point>\n"
                               "    <point x=\"705\" y=\"360\" lat=\"46.031397879\" lon=\"13.186903596\">LIPD</point>\n"
                               "    <point x=\"804\" y=\"332\" lat=\"46.223685443\" lon=\"14.457608834\">LJLJ</point>\n"
                               "    <point x=\"789\" y=\"285\" lat=\"46.643188298\" lon=\"14.324599430\">LOWK</point>\n"
                               "    <point x=\"870\" y=\"239\" lat=\"46.991068125\" lon=\"15.439625904\">LOWG</point>\n"
                               "    <point x=\"559\" y=\"227\" lat=\"47.260225117\" lon=\"11.343967170\">LOWI</point>\n"
                               "    <point x=\"681\" y=\"163\" lat=\"47.793332934\" lon=\"13.004333675\">LOWS</point>\n"
                               "    <point x=\"765\" y=\"110\" lat=\"48.233220577\" lon=\"14.187513739\">LOWL</point>\n"
                               "    <point x=\"942\" y=\"106\" lat=\"48.115932941\" lon=\"16.554506868\">LOWW</point>\n"
                               "    <point x=\"589\" y=\"104\" lat=\"48.342751265\" lon=\"11.777872816\">EDDM</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Deutschland Süd",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/ri_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_ri_{0}.png"),
                               "%y%m%d%H%M",300,0,1027,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_ri_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Deutschland Süd {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"11.5\">\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"684\" y=\"546\">Muenchen</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"401\" y=\"249\">Frankfurt</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"631\" y=\"352\">Nuernberg</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"951\" y=\"243\">Prag</point>\n"
                               "    <point lat=\"47.368652\" lon=\"8.539188\"  x=\"377\" y=\"662\">Zuerich</point>\n"
                               "    <point lat=\"49.240158\" lon=\"6.996939\"  x=\"233\" y=\"377\">Saarbruecken</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"444\" y=\"453\">Stuttgart</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"246\" y=\"120\">Koeln</point>\n"
                               "    <point lat=\"51.050405\" lon=\"13.737266\" x=\"873\" y=\"101\">Dresden</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"747\" y=\"67\">Leipzig</point>\n"
                               "    <point lat=\"47.557422\" lon=\"7.592572\"  x=\"282\" y=\"631\">Basel</point>\n"
                               "    <point lat=\"47.809494\" lon=\"13.055019\"  x=\"835\" y=\"592\">Salzburg</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Deutschland Mitte",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/rd_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rd_{0}.png"),
                               "%y%m%d%H%M",300,0,974,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rd_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Deutschland Mitte {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10.5\">\n"
                               "    <point lat=\"48.371446\" lon=\"10.898244\" x=\"617\" y=\"687\">Augsburg</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"401\" y=\"421\">Frankfurt</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"629\" y=\"522\">Nuernberg</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"950\" y=\"414\">Prag</point>\n"
                               "    <point lat=\"49.240158\" lon=\"6.996939\"  x=\"233\" y=\"549\">Saarbruecken</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"448\" y=\"624\">Stuttgart</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"246\" y=\"296\">Koeln</point>\n"
                               "    <point lat=\"51.050405\" lon=\"13.737266\" x=\"874\" y=\"273\">Dresden</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"747\" y=\"238\">Leipzig</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"832\" y=\"62\">Berlin</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"502\" y=\"88\">Hannover</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Deutschland Nord",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/ra_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_ra_{0}.png"),
                               "%y%m%d%H%M",300,0,781,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_ra_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Deutschland Nord {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10.0\">\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"951\" y=\"697\">Prag</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"246\" y=\"572\">Koeln</point>\n"
                               "    <point lat=\"51.050405\" lon=\"13.737266\" x=\"874\" y=\"555\">Dresden</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"747\" y=\"518\">Leipzig</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"830\" y=\"341\">Berlin</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"504\" y=\"368\">Hannover</point>\n"
                               "    <point lat=\"50.360028\" lon=\"7.5899\"    x=\"291\" y=\"651\">Koblenz</point>\n"
                               "    <point lat=\"53.079306\" lon=\"8.80168\"   x=\"417\" y=\"265\">Bremen</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"528\" y=\"195\">Hamburg</point>\n"
                               "    <point lat=\"54.323294\" lon=\"10.122764\" x=\"539\" y=\"86\">Kiel</point>\n"
                               "    <point lat=\"54.793745\" lon=\"9.446998\"  x=\"481\" y=\"15\">Flensburg</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Lightning Europe 60min",
                               "http://www.flugwetter.de/scripts/karte.php?format=jpg&maxage=50&lan=de",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/lightning_60_{0}.jpg"),
                               "%y%m%d%H%M",60,0,177,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/lightning_60_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Lightning Europe 60min {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"6\">\n"
                               "    <point lat=\"53.349808\" lon=\"-6.260319\" x=\"102\" y=\"135\">Dublin</point>\n"
                               "    <point lat=\"53.479325\" lon=\"-2.248485\" x=\"222\" y=\"153\">Manchester</point>\n"
                               "    <point lat=\"51.511226\" lon=\"-0.11982\"  x=\"271\" y=\"260\">London</point>\n"
                               "    <point lat=\"49.494375\" lon=\"0.107932\"  x=\"263\" y=\"360\">Le Havre</point>\n"
                               "    <point lat=\"48.856612\" lon=\"2.352211\"  x=\"331\" y=\"401\">Paris</point>\n"
                               "    <point lat=\"48.390401\" lon=\"-4.48607\"  x=\"107\" y=\"387\">Brest</point>\n"
                               "    <point lat=\"44.837795\" lon=\"-0.579185\" x=\"206\" y=\"589\">Bordeaux</point>\n"
                               "    <point lat=\"45.76405\"  lon=\"4.835658\"  x=\"402\" y=\"565\">Lyon</point>\n"
                               "    <point lat=\"43.60465\"  lon=\"1.444209\"  x=\"270\" y=\"659\">Toulouse</point>\n"
                               "    <point lat=\"43.29648\"  lon=\"5.369782\"  x=\"413\" y=\"689\">Marseille</point>\n"
                               "    <point lat=\"41.385068\" lon=\"2.173405\"  x=\"284\" y=\"774\">Barcelona</point>\n"
                               "    <point lat=\"40.41678\"  lon=\"-3.703787\" x=\"54\"  y=\"789\">Madrid</point>\n"
                               "    <point lat=\"46.947919\" lon=\"7.444611\"  x=\"497\" y=\"510\">Bern</point>\n"
                               "    <point lat=\"47.368652\" lon=\"8.539188\"  x=\"533\" y=\"490\">Zuerich</point>\n"
                               "    <point lat=\"45.465458\" lon=\"9.186523\"  x=\"555\" y=\"586\">Mailand</point>\n"
                               "    <point lat=\"43.771032\" lon=\"11.248004\" x=\"628\" y=\"672\">Florenz</point>\n"
                               "    <point lat=\"41.892918\" lon=\"12.482529\" x=\"676\" y=\"765\">Rom</point>\n"
                               "    <point lat=\"46.056452\" lon=\"14.508069\" x=\"740\" y=\"552\">Ljubljana</point>\n"
                               "    <point lat=\"45.813023\" lon=\"15.977898\" x=\"793\" y=\"562\">Zagreb</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"559\" y=\"425\">Stuttgart</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"637\" y=\"451\">Muenchen</point>\n"
                               "    <point lat=\"48.208173\" lon=\"16.373813\" x=\"797\" y=\"441\">Wien</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"619\" y=\"384\">Nuernberg</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"728\" y=\"352\">Prag</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"542\" y=\"353\">Frankfurt</point>\n"
                               "    <point lat=\"49.611628\" lon=\"6.131938\"  x=\"461\" y=\"374\">Luxemburg</point>\n"
                               "    <point lat=\"50.85035\"  lon=\"4.351723\"  x=\"405\" y=\"309\">Bruessel</point>\n"
                               "    <point lat=\"52.370215\" lon=\"4.89516\"   x=\"429\" y=\"233\">Amsterdam</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"495\" y=\"313\">Koeln</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"577\" y=\"240\">Hannover</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"586\" y=\"181\">Hamburg</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"688\" y=\"230\">Berlin</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"655\" y=\"287\">Leipzig</point>\n"
                               "    <point lat=\"51.050405\" lon=\"13.737266\" x=\"703\" y=\"300\">Dresden</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Lightning Europe 50min",
                               "http://www.flugwetter.de/scripts/karte.php?format=jpg&maxage=40&lan=de",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/lightning_50_{0}.jpg"),
                               "%y%m%d%H%M",60,0,177,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/lightning_50_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Lightning Europe 60min {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"6\">\n"
                               "    <point lat=\"53.349808\" lon=\"-6.260319\" x=\"102\" y=\"135\">Dublin</point>\n"
                               "    <point lat=\"53.479325\" lon=\"-2.248485\" x=\"222\" y=\"153\">Manchester</point>\n"
                               "    <point lat=\"51.511226\" lon=\"-0.11982\"  x=\"271\" y=\"260\">London</point>\n"
                               "    <point lat=\"49.494375\" lon=\"0.107932\"  x=\"263\" y=\"360\">Le Havre</point>\n"
                               "    <point lat=\"48.856612\" lon=\"2.352211\"  x=\"331\" y=\"401\">Paris</point>\n"
                               "    <point lat=\"48.390401\" lon=\"-4.48607\"  x=\"107\" y=\"387\">Brest</point>\n"
                               "    <point lat=\"44.837795\" lon=\"-0.579185\" x=\"206\" y=\"589\">Bordeaux</point>\n"
                               "    <point lat=\"45.76405\"  lon=\"4.835658\"  x=\"402\" y=\"565\">Lyon</point>\n"
                               "    <point lat=\"43.60465\"  lon=\"1.444209\"  x=\"270\" y=\"659\">Toulouse</point>\n"
                               "    <point lat=\"43.29648\"  lon=\"5.369782\"  x=\"413\" y=\"689\">Marseille</point>\n"
                               "    <point lat=\"41.385068\" lon=\"2.173405\"  x=\"284\" y=\"774\">Barcelona</point>\n"
                               "    <point lat=\"40.41678\"  lon=\"-3.703787\" x=\"54\"  y=\"789\">Madrid</point>\n"
                               "    <point lat=\"46.947919\" lon=\"7.444611\"  x=\"497\" y=\"510\">Bern</point>\n"
                               "    <point lat=\"47.368652\" lon=\"8.539188\"  x=\"533\" y=\"490\">Zuerich</point>\n"
                               "    <point lat=\"45.465458\" lon=\"9.186523\"  x=\"555\" y=\"586\">Mailand</point>\n"
                               "    <point lat=\"43.771032\" lon=\"11.248004\" x=\"628\" y=\"672\">Florenz</point>\n"
                               "    <point lat=\"41.892918\" lon=\"12.482529\" x=\"676\" y=\"765\">Rom</point>\n"
                               "    <point lat=\"46.056452\" lon=\"14.508069\" x=\"740\" y=\"552\">Ljubljana</point>\n"
                               "    <point lat=\"45.813023\" lon=\"15.977898\" x=\"793\" y=\"562\">Zagreb</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"559\" y=\"425\">Stuttgart</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"637\" y=\"451\">Muenchen</point>\n"
                               "    <point lat=\"48.208173\" lon=\"16.373813\" x=\"797\" y=\"441\">Wien</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"619\" y=\"384\">Nuernberg</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"728\" y=\"352\">Prag</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"542\" y=\"353\">Frankfurt</point>\n"
                               "    <point lat=\"49.611628\" lon=\"6.131938\"  x=\"461\" y=\"374\">Luxemburg</point>\n"
                               "    <point lat=\"50.85035\"  lon=\"4.351723\"  x=\"405\" y=\"309\">Bruessel</point>\n"
                               "    <point lat=\"52.370215\" lon=\"4.89516\"   x=\"429\" y=\"233\">Amsterdam</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"495\" y=\"313\">Koeln</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"577\" y=\"240\">Hannover</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"586\" y=\"181\">Hamburg</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"688\" y=\"230\">Berlin</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"655\" y=\"287\">Leipzig</point>\n"
                               "    <point lat=\"51.050405\" lon=\"13.737266\" x=\"703\" y=\"300\">Dresden</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Lightning Europe 40min",
                               "http://www.flugwetter.de/scripts/karte.php?format=jpg&maxage=30&lan=de",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/lightning_40_{0}.jpg"),
                               "%y%m%d%H%M",60,0,177,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/lightning_40_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Lightning Europe 60min {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"6\">\n"
                               "    <point lat=\"53.349808\" lon=\"-6.260319\" x=\"102\" y=\"135\">Dublin</point>\n"
                               "    <point lat=\"53.479325\" lon=\"-2.248485\" x=\"222\" y=\"153\">Manchester</point>\n"
                               "    <point lat=\"51.511226\" lon=\"-0.11982\"  x=\"271\" y=\"260\">London</point>\n"
                               "    <point lat=\"49.494375\" lon=\"0.107932\"  x=\"263\" y=\"360\">Le Havre</point>\n"
                               "    <point lat=\"48.856612\" lon=\"2.352211\"  x=\"331\" y=\"401\">Paris</point>\n"
                               "    <point lat=\"48.390401\" lon=\"-4.48607\"  x=\"107\" y=\"387\">Brest</point>\n"
                               "    <point lat=\"44.837795\" lon=\"-0.579185\" x=\"206\" y=\"589\">Bordeaux</point>\n"
                               "    <point lat=\"45.76405\"  lon=\"4.835658\"  x=\"402\" y=\"565\">Lyon</point>\n"
                               "    <point lat=\"43.60465\"  lon=\"1.444209\"  x=\"270\" y=\"659\">Toulouse</point>\n"
                               "    <point lat=\"43.29648\"  lon=\"5.369782\"  x=\"413\" y=\"689\">Marseille</point>\n"
                               "    <point lat=\"41.385068\" lon=\"2.173405\"  x=\"284\" y=\"774\">Barcelona</point>\n"
                               "    <point lat=\"40.41678\"  lon=\"-3.703787\" x=\"54\"  y=\"789\">Madrid</point>\n"
                               "    <point lat=\"46.947919\" lon=\"7.444611\"  x=\"497\" y=\"510\">Bern</point>\n"
                               "    <point lat=\"47.368652\" lon=\"8.539188\"  x=\"533\" y=\"490\">Zuerich</point>\n"
                               "    <point lat=\"45.465458\" lon=\"9.186523\"  x=\"555\" y=\"586\">Mailand</point>\n"
                               "    <point lat=\"43.771032\" lon=\"11.248004\" x=\"628\" y=\"672\">Florenz</point>\n"
                               "    <point lat=\"41.892918\" lon=\"12.482529\" x=\"676\" y=\"765\">Rom</point>\n"
                               "    <point lat=\"46.056452\" lon=\"14.508069\" x=\"740\" y=\"552\">Ljubljana</point>\n"
                               "    <point lat=\"45.813023\" lon=\"15.977898\" x=\"793\" y=\"562\">Zagreb</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"559\" y=\"425\">Stuttgart</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"637\" y=\"451\">Muenchen</point>\n"
                               "    <point lat=\"48.208173\" lon=\"16.373813\" x=\"797\" y=\"441\">Wien</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"619\" y=\"384\">Nuernberg</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"728\" y=\"352\">Prag</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"542\" y=\"353\">Frankfurt</point>\n"
                               "    <point lat=\"49.611628\" lon=\"6.131938\"  x=\"461\" y=\"374\">Luxemburg</point>\n"
                               "    <point lat=\"50.85035\"  lon=\"4.351723\"  x=\"405\" y=\"309\">Bruessel</point>\n"
                               "    <point lat=\"52.370215\" lon=\"4.89516\"   x=\"429\" y=\"233\">Amsterdam</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"495\" y=\"313\">Koeln</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"577\" y=\"240\">Hannover</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"586\" y=\"181\">Hamburg</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"688\" y=\"230\">Berlin</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"655\" y=\"287\">Leipzig</point>\n"
                               "    <point lat=\"51.050405\" lon=\"13.737266\" x=\"703\" y=\"300\">Dresden</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Lightning Europe 30min",
                               "http://www.flugwetter.de/scripts/karte.php?format=jpg&maxage=20&lan=de",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/lightning_30_{0}.jpg"),
                               "%y%m%d%H%M",60,0,177,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/lightning_30_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Lightning Europe 60min {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"6\">\n"
                               "    <point lat=\"53.349808\" lon=\"-6.260319\" x=\"102\" y=\"135\">Dublin</point>\n"
                               "    <point lat=\"53.479325\" lon=\"-2.248485\" x=\"222\" y=\"153\">Manchester</point>\n"
                               "    <point lat=\"51.511226\" lon=\"-0.11982\"  x=\"271\" y=\"260\">London</point>\n"
                               "    <point lat=\"49.494375\" lon=\"0.107932\"  x=\"263\" y=\"360\">Le Havre</point>\n"
                               "    <point lat=\"48.856612\" lon=\"2.352211\"  x=\"331\" y=\"401\">Paris</point>\n"
                               "    <point lat=\"48.390401\" lon=\"-4.48607\"  x=\"107\" y=\"387\">Brest</point>\n"
                               "    <point lat=\"44.837795\" lon=\"-0.579185\" x=\"206\" y=\"589\">Bordeaux</point>\n"
                               "    <point lat=\"45.76405\"  lon=\"4.835658\"  x=\"402\" y=\"565\">Lyon</point>\n"
                               "    <point lat=\"43.60465\"  lon=\"1.444209\"  x=\"270\" y=\"659\">Toulouse</point>\n"
                               "    <point lat=\"43.29648\"  lon=\"5.369782\"  x=\"413\" y=\"689\">Marseille</point>\n"
                               "    <point lat=\"41.385068\" lon=\"2.173405\"  x=\"284\" y=\"774\">Barcelona</point>\n"
                               "    <point lat=\"40.41678\"  lon=\"-3.703787\" x=\"54\"  y=\"789\">Madrid</point>\n"
                               "    <point lat=\"46.947919\" lon=\"7.444611\"  x=\"497\" y=\"510\">Bern</point>\n"
                               "    <point lat=\"47.368652\" lon=\"8.539188\"  x=\"533\" y=\"490\">Zuerich</point>\n"
                               "    <point lat=\"45.465458\" lon=\"9.186523\"  x=\"555\" y=\"586\">Mailand</point>\n"
                               "    <point lat=\"43.771032\" lon=\"11.248004\" x=\"628\" y=\"672\">Florenz</point>\n"
                               "    <point lat=\"41.892918\" lon=\"12.482529\" x=\"676\" y=\"765\">Rom</point>\n"
                               "    <point lat=\"46.056452\" lon=\"14.508069\" x=\"740\" y=\"552\">Ljubljana</point>\n"
                               "    <point lat=\"45.813023\" lon=\"15.977898\" x=\"793\" y=\"562\">Zagreb</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"559\" y=\"425\">Stuttgart</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"637\" y=\"451\">Muenchen</point>\n"
                               "    <point lat=\"48.208173\" lon=\"16.373813\" x=\"797\" y=\"441\">Wien</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"619\" y=\"384\">Nuernberg</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"728\" y=\"352\">Prag</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"542\" y=\"353\">Frankfurt</point>\n"
                               "    <point lat=\"49.611628\" lon=\"6.131938\"  x=\"461\" y=\"374\">Luxemburg</point>\n"
                               "    <point lat=\"50.85035\"  lon=\"4.351723\"  x=\"405\" y=\"309\">Bruessel</point>\n"
                               "    <point lat=\"52.370215\" lon=\"4.89516\"   x=\"429\" y=\"233\">Amsterdam</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"495\" y=\"313\">Koeln</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"577\" y=\"240\">Hannover</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"586\" y=\"181\">Hamburg</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"688\" y=\"230\">Berlin</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"655\" y=\"287\">Leipzig</point>\n"
                               "    <point lat=\"51.050405\" lon=\"13.737266\" x=\"703\" y=\"300\">Dresden</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Lightning Europe 20min",
                               "http://www.flugwetter.de/scripts/karte.php?format=jpg&maxage=10&lan=de",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/lightning_20_{0}.jpg"),
                               "%y%m%d%H%M",60,0,177,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/lightning_20_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Lightning Europe 60min {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"6\">\n"
                               "    <point lat=\"53.349808\" lon=\"-6.260319\" x=\"102\" y=\"135\">Dublin</point>\n"
                               "    <point lat=\"53.479325\" lon=\"-2.248485\" x=\"222\" y=\"153\">Manchester</point>\n"
                               "    <point lat=\"51.511226\" lon=\"-0.11982\"  x=\"271\" y=\"260\">London</point>\n"
                               "    <point lat=\"49.494375\" lon=\"0.107932\"  x=\"263\" y=\"360\">Le Havre</point>\n"
                               "    <point lat=\"48.856612\" lon=\"2.352211\"  x=\"331\" y=\"401\">Paris</point>\n"
                               "    <point lat=\"48.390401\" lon=\"-4.48607\"  x=\"107\" y=\"387\">Brest</point>\n"
                               "    <point lat=\"44.837795\" lon=\"-0.579185\" x=\"206\" y=\"589\">Bordeaux</point>\n"
                               "    <point lat=\"45.76405\"  lon=\"4.835658\"  x=\"402\" y=\"565\">Lyon</point>\n"
                               "    <point lat=\"43.60465\"  lon=\"1.444209\"  x=\"270\" y=\"659\">Toulouse</point>\n"
                               "    <point lat=\"43.29648\"  lon=\"5.369782\"  x=\"413\" y=\"689\">Marseille</point>\n"
                               "    <point lat=\"41.385068\" lon=\"2.173405\"  x=\"284\" y=\"774\">Barcelona</point>\n"
                               "    <point lat=\"40.41678\"  lon=\"-3.703787\" x=\"54\"  y=\"789\">Madrid</point>\n"
                               "    <point lat=\"46.947919\" lon=\"7.444611\"  x=\"497\" y=\"510\">Bern</point>\n"
                               "    <point lat=\"47.368652\" lon=\"8.539188\"  x=\"533\" y=\"490\">Zuerich</point>\n"
                               "    <point lat=\"45.465458\" lon=\"9.186523\"  x=\"555\" y=\"586\">Mailand</point>\n"
                               "    <point lat=\"43.771032\" lon=\"11.248004\" x=\"628\" y=\"672\">Florenz</point>\n"
                               "    <point lat=\"41.892918\" lon=\"12.482529\" x=\"676\" y=\"765\">Rom</point>\n"
                               "    <point lat=\"46.056452\" lon=\"14.508069\" x=\"740\" y=\"552\">Ljubljana</point>\n"
                               "    <point lat=\"45.813023\" lon=\"15.977898\" x=\"793\" y=\"562\">Zagreb</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"559\" y=\"425\">Stuttgart</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"637\" y=\"451\">Muenchen</point>\n"
                               "    <point lat=\"48.208173\" lon=\"16.373813\" x=\"797\" y=\"441\">Wien</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"619\" y=\"384\">Nuernberg</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"728\" y=\"352\">Prag</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"542\" y=\"353\">Frankfurt</point>\n"
                               "    <point lat=\"49.611628\" lon=\"6.131938\"  x=\"461\" y=\"374\">Luxemburg</point>\n"
                               "    <point lat=\"50.85035\"  lon=\"4.351723\"  x=\"405\" y=\"309\">Bruessel</point>\n"
                               "    <point lat=\"52.370215\" lon=\"4.89516\"   x=\"429\" y=\"233\">Amsterdam</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"495\" y=\"313\">Koeln</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"577\" y=\"240\">Hannover</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"586\" y=\"181\">Hamburg</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"688\" y=\"230\">Berlin</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"655\" y=\"287\">Leipzig</point>\n"
                               "    <point lat=\"51.050405\" lon=\"13.737266\" x=\"703\" y=\"300\">Dresden</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Lightning Europe 10min",
                               "http://www.flugwetter.de/scripts/karte.php?format=jpg&maxage=0&lan=de",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/lightning_10_{0}.jpg"),
                               "%y%m%d%H%M",60,0,177,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/lightning_10_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Lightning Europe 60min {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"6\">\n"
                               "    <point lat=\"53.349808\" lon=\"-6.260319\" x=\"102\" y=\"135\">Dublin</point>\n"
                               "    <point lat=\"53.479325\" lon=\"-2.248485\" x=\"222\" y=\"153\">Manchester</point>\n"
                               "    <point lat=\"51.511226\" lon=\"-0.11982\"  x=\"271\" y=\"260\">London</point>\n"
                               "    <point lat=\"49.494375\" lon=\"0.107932\"  x=\"263\" y=\"360\">Le Havre</point>\n"
                               "    <point lat=\"48.856612\" lon=\"2.352211\"  x=\"331\" y=\"401\">Paris</point>\n"
                               "    <point lat=\"48.390401\" lon=\"-4.48607\"  x=\"107\" y=\"387\">Brest</point>\n"
                               "    <point lat=\"44.837795\" lon=\"-0.579185\" x=\"206\" y=\"589\">Bordeaux</point>\n"
                               "    <point lat=\"45.76405\"  lon=\"4.835658\"  x=\"402\" y=\"565\">Lyon</point>\n"
                               "    <point lat=\"43.60465\"  lon=\"1.444209\"  x=\"270\" y=\"659\">Toulouse</point>\n"
                               "    <point lat=\"43.29648\"  lon=\"5.369782\"  x=\"413\" y=\"689\">Marseille</point>\n"
                               "    <point lat=\"41.385068\" lon=\"2.173405\"  x=\"284\" y=\"774\">Barcelona</point>\n"
                               "    <point lat=\"40.41678\"  lon=\"-3.703787\" x=\"54\"  y=\"789\">Madrid</point>\n"
                               "    <point lat=\"46.947919\" lon=\"7.444611\"  x=\"497\" y=\"510\">Bern</point>\n"
                               "    <point lat=\"47.368652\" lon=\"8.539188\"  x=\"533\" y=\"490\">Zuerich</point>\n"
                               "    <point lat=\"45.465458\" lon=\"9.186523\"  x=\"555\" y=\"586\">Mailand</point>\n"
                               "    <point lat=\"43.771032\" lon=\"11.248004\" x=\"628\" y=\"672\">Florenz</point>\n"
                               "    <point lat=\"41.892918\" lon=\"12.482529\" x=\"676\" y=\"765\">Rom</point>\n"
                               "    <point lat=\"46.056452\" lon=\"14.508069\" x=\"740\" y=\"552\">Ljubljana</point>\n"
                               "    <point lat=\"45.813023\" lon=\"15.977898\" x=\"793\" y=\"562\">Zagreb</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"559\" y=\"425\">Stuttgart</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"637\" y=\"451\">Muenchen</point>\n"
                               "    <point lat=\"48.208173\" lon=\"16.373813\" x=\"797\" y=\"441\">Wien</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"619\" y=\"384\">Nuernberg</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"728\" y=\"352\">Prag</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"542\" y=\"353\">Frankfurt</point>\n"
                               "    <point lat=\"49.611628\" lon=\"6.131938\"  x=\"461\" y=\"374\">Luxemburg</point>\n"
                               "    <point lat=\"50.85035\"  lon=\"4.351723\"  x=\"405\" y=\"309\">Bruessel</point>\n"
                               "    <point lat=\"52.370215\" lon=\"4.89516\"   x=\"429\" y=\"233\">Amsterdam</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"495\" y=\"313\">Koeln</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"577\" y=\"240\">Hannover</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"586\" y=\"181\">Hamburg</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"688\" y=\"230\">Berlin</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"655\" y=\"287\">Leipzig</point>\n"
                               "    <point lat=\"51.050405\" lon=\"13.737266\" x=\"703\" y=\"300\">Dresden</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Satellite Europe HRV",
                               "http://www.flugwetter.de/scripts/getimg.php?src=sat/mete_std_12-12-1_europa_{0}_sat.jpg",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_12-12-1_europa_{0}_sat.jpg"),
                               "%y%m%d%H%M",300,0,917,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_12-12-1_europa_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Satellite Europe HRV {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"12\">\n"
                               "    <point lat=\"40.41678\"  lon=\"-3.703787\"  x=\"413\"  y=\"713\">Madrid</point>\n"
                               "    <point lat=\"38.725296\" lon=\"-9.150045\"  x=\"299\"  y=\"723\">Lissabon</point>\n"
                               "    <point lat=\"34.01505\"  lon=\"-6.832724\"  x=\"307\"  y=\"857\">Rabat</point>\n"
                               "    <point lat=\"36.752889\" lon=\"3.042043\"   x=\"530\"  y=\"830\">Algiers</point>\n"
                               "    <point lat=\"36.818802\" lon=\"10.165958\"  x=\"682\"  y=\"838\">Tunis</point>\n"
                               "    <point lat=\"32.876199\" lon=\"13.187542\"  x=\"751\"  y=\"941\">Tripoli</point>\n"
                               "    <point lat=\"30.044411\" lon=\"31.235719\"  x=\"1182\" y=\"927\">Cairo</point>\n"
                               "    <point lat=\"36.043248\" lon=\"14.242213\"  x=\"773\"  y=\"860\">Victoria</point>\n"
                               "    <point lat=\"35.338733\" lon=\"25.144218\"  x=\"1002\" y=\"833\">Iraklion</point>\n"
                               "    <point lat=\"37.983716\" lon=\"23.72931\"   x=\"953\"  y=\"773\">Athens</point>\n"
                               "    <point lat=\"39.920763\" lon=\"32.854099\"  x=\"1115\" y=\"669\">Ankara</point>\n"
                               "    <point lat=\"42.697829\" lon=\"23.321657\"  x=\"920\"  y=\"656\">Sofia</point>\n"
                               "    <point lat=\"41.331645\" lon=\"19.831803\"  x=\"863\"  y=\"706\">Tirana</point>\n"
                               "    <point lat=\"43.856254\" lon=\"18.413086\"  x=\"827\"  y=\"646\">Sarajevo</point>\n"
                               "    <point lat=\"44.820553\" lon=\"20.462229\"  x=\"860\"  y=\"616\">Belgrade</point>\n"
                               "    <point lat=\"41.892918\" lon=\"12.482529\"  x=\"725\"  y=\"706\">Rom</point>\n"
                               "    <point lat=\"45.813023\" lon=\"15.977898\"  x=\"780\"  y=\"604\">Zagreb</point>\n"
                               "    <point lat=\"46.947919\" lon=\"7.444611\"   x=\"635\"  y=\"581\">Bern</point>\n"
                               "    <point lat=\"48.208173\" lon=\"16.373813\"  x=\"780\"  y=\"546\">Wien</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\"  x=\"745\"  y=\"504\">Prag</point>\n"
                               "    <point lat=\"48.145888\" lon=\"17.107129\"  x=\"826\"  y=\"556\">Bratislava</point>\n"
                               "    <point lat=\"44.432493\" lon=\"26.10388\"   x=\"960\"  y=\"603\">Bucharest</point>\n"
                               "    <point lat=\"46.48458\"  lon=\"30.732601\"  x=\"1020\" y=\"532\">Odesa</point>\n"
                               "    <point lat=\"50.450099\" lon=\"30.52341\"   x=\"980\"  y=\"442\">Kiev</point>\n"
                               "    <point lat=\"55.755834\" lon=\"37.617316\"  x=\"1022\" y=\"290\">Moscow</point>\n"
                               "    <point lat=\"53.900002\" lon=\"27.566671\"  x=\"915\"  y=\"379\">Minsk</point>\n"
                               "    <point lat=\"52.229679\" lon=\"21.012232\"  x=\"835\"  y=\"440\">Warsaw</point>\n"
                               "    <point lat=\"56.949644\" lon=\"24.105184\"  x=\"850\"  y=\"325\">Riga</point>\n"
                               "    <point lat=\"60.173324\" lon=\"24.941025\"  x=\"844\"  y=\"252\">Helsinki</point>\n"
                               "    <point lat=\"59.328921\" lon=\"18.064914\"  x=\"769\"  y=\"286\">Stockholm</point>\n"
                               "    <point lat=\"59.91387\"  lon=\"10.752246\"  x=\"686\"  y=\"279\">Oslo</point>\n"
                               "    <point lat=\"55.676096\" lon=\"12.568338\"  x=\"711\"  y=\"373\">Kopenhagen</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"   x=\"726\"  y=\"448\">Berlin</point>\n"
                               "    <point lat=\"50.85035\"  lon=\"4.351723\"   x=\"593\"  y=\"485\">Bruessel</point>\n"
                               "    <point lat=\"48.856612\" lon=\"2.352211\"   x=\"557\"  y=\"528\">Paris</point>\n"
                               "    <point lat=\"51.511226\" lon=\"-0.11982\"   x=\"529\"  y=\"460\">London</point>\n"
                               "    <point lat=\"53.349808\" lon=\"-6.260319\"  x=\"454\"  y=\"398\">Dublin</point>\n"
                               "    <point lat=\"64.135345\" lon=\"-21.895194\" x=\"385\"  y=\"101\">Rejkjavik</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Satellite Europe RGB",
                               "http://www.flugwetter.de/scripts/getimg.php?src=sat/mete_std_1-12-9i_europa_{0}_sat.jpg",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_1-12-9i_europa_{0}_sat.jpg"),
                               "%y%m%d%H%M",300,0,1119,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_1-12-9i_europa_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Satellite Europe RGB {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"12\">\n"
                               "    <point lat=\"40.41678\"  lon=\"-3.703787\"  x=\"413\"  y=\"713\">Madrid</point>\n"
                               "    <point lat=\"38.725296\" lon=\"-9.150045\"  x=\"299\"  y=\"723\">Lissabon</point>\n"
                               "    <point lat=\"34.01505\"  lon=\"-6.832724\"  x=\"307\"  y=\"857\">Rabat</point>\n"
                               "    <point lat=\"36.752889\" lon=\"3.042043\"   x=\"530\"  y=\"830\">Algiers</point>\n"
                               "    <point lat=\"36.818802\" lon=\"10.165958\"  x=\"682\"  y=\"838\">Tunis</point>\n"
                               "    <point lat=\"32.876199\" lon=\"13.187542\"  x=\"751\"  y=\"941\">Tripoli</point>\n"
                               "    <point lat=\"30.044411\" lon=\"31.235719\"  x=\"1182\" y=\"927\">Cairo</point>\n"
                               "    <point lat=\"36.043248\" lon=\"14.242213\"  x=\"773\"  y=\"860\">Victoria</point>\n"
                               "    <point lat=\"35.338733\" lon=\"25.144218\"  x=\"1002\" y=\"833\">Iraklion</point>\n"
                               "    <point lat=\"37.983716\" lon=\"23.72931\"   x=\"953\"  y=\"773\">Athens</point>\n"
                               "    <point lat=\"39.920763\" lon=\"32.854099\"  x=\"1115\" y=\"669\">Ankara</point>\n"
                               "    <point lat=\"42.697829\" lon=\"23.321657\"  x=\"920\"  y=\"656\">Sofia</point>\n"
                               "    <point lat=\"41.331645\" lon=\"19.831803\"  x=\"863\"  y=\"706\">Tirana</point>\n"
                               "    <point lat=\"43.856254\" lon=\"18.413086\"  x=\"827\"  y=\"646\">Sarajevo</point>\n"
                               "    <point lat=\"44.820553\" lon=\"20.462229\"  x=\"860\"  y=\"616\">Belgrade</point>\n"
                               "    <point lat=\"41.892918\" lon=\"12.482529\"  x=\"725\"  y=\"706\">Rom</point>\n"
                               "    <point lat=\"45.813023\" lon=\"15.977898\"  x=\"780\"  y=\"604\">Zagreb</point>\n"
                               "    <point lat=\"46.947919\" lon=\"7.444611\"   x=\"635\"  y=\"581\">Bern</point>\n"
                               "    <point lat=\"48.208173\" lon=\"16.373813\"  x=\"780\"  y=\"546\">Wien</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\"  x=\"745\"  y=\"504\">Prag</point>\n"
                               "    <point lat=\"48.145888\" lon=\"17.107129\"  x=\"826\"  y=\"556\">Bratislava</point>\n"
                               "    <point lat=\"44.432493\" lon=\"26.10388\"   x=\"960\"  y=\"603\">Bucharest</point>\n"
                               "    <point lat=\"46.48458\"  lon=\"30.732601\"  x=\"1020\" y=\"532\">Odesa</point>\n"
                               "    <point lat=\"50.450099\" lon=\"30.52341\"   x=\"980\"  y=\"442\">Kiev</point>\n"
                               "    <point lat=\"55.755834\" lon=\"37.617316\"  x=\"1022\" y=\"290\">Moscow</point>\n"
                               "    <point lat=\"53.900002\" lon=\"27.566671\"  x=\"915\"  y=\"379\">Minsk</point>\n"
                               "    <point lat=\"52.229679\" lon=\"21.012232\"  x=\"835\"  y=\"440\">Warsaw</point>\n"
                               "    <point lat=\"56.949644\" lon=\"24.105184\"  x=\"850\"  y=\"325\">Riga</point>\n"
                               "    <point lat=\"60.173324\" lon=\"24.941025\"  x=\"844\"  y=\"252\">Helsinki</point>\n"
                               "    <point lat=\"59.328921\" lon=\"18.064914\"  x=\"769\"  y=\"286\">Stockholm</point>\n"
                               "    <point lat=\"59.91387\"  lon=\"10.752246\"  x=\"686\"  y=\"279\">Oslo</point>\n"
                               "    <point lat=\"55.676096\" lon=\"12.568338\"  x=\"711\"  y=\"373\">Kopenhagen</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"   x=\"726\"  y=\"448\">Berlin</point>\n"
                               "    <point lat=\"50.85035\"  lon=\"4.351723\"   x=\"593\"  y=\"485\">Bruessel</point>\n"
                               "    <point lat=\"48.856612\" lon=\"2.352211\"   x=\"557\"  y=\"528\">Paris</point>\n"
                               "    <point lat=\"51.511226\" lon=\"-0.11982\"   x=\"529\"  y=\"460\">London</point>\n"
                               "    <point lat=\"53.349808\" lon=\"-6.260319\"  x=\"454\"  y=\"398\">Dublin</point>\n"
                               "    <point lat=\"64.135345\" lon=\"-21.895194\" x=\"385\"  y=\"101\">Rejkjavik</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Satellite Europe IR",
                               "http://www.flugwetter.de/scripts/getimg.php?src=sat/mete_std_ir108_europa_{0}_sat.jpg",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_ir108_europa_{0}_sat.jpg"),
                               "%y%m%d%H%M",300,0,925,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_ir108_europa_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Satellite Europe IR {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"12\">\n"
                               "    <point lat=\"40.41678\"  lon=\"-3.703787\"  x=\"413\"  y=\"713\">Madrid</point>\n"
                               "    <point lat=\"38.725296\" lon=\"-9.150045\"  x=\"299\"  y=\"723\">Lissabon</point>\n"
                               "    <point lat=\"34.01505\"  lon=\"-6.832724\"  x=\"307\"  y=\"857\">Rabat</point>\n"
                               "    <point lat=\"36.752889\" lon=\"3.042043\"   x=\"530\"  y=\"830\">Algiers</point>\n"
                               "    <point lat=\"36.818802\" lon=\"10.165958\"  x=\"682\"  y=\"838\">Tunis</point>\n"
                               "    <point lat=\"32.876199\" lon=\"13.187542\"  x=\"751\"  y=\"941\">Tripoli</point>\n"
                               "    <point lat=\"30.044411\" lon=\"31.235719\"  x=\"1182\" y=\"927\">Cairo</point>\n"
                               "    <point lat=\"36.043248\" lon=\"14.242213\"  x=\"773\"  y=\"860\">Victoria</point>\n"
                               "    <point lat=\"35.338733\" lon=\"25.144218\"  x=\"1002\" y=\"833\">Iraklion</point>\n"
                               "    <point lat=\"37.983716\" lon=\"23.72931\"   x=\"953\"  y=\"773\">Athens</point>\n"
                               "    <point lat=\"39.920763\" lon=\"32.854099\"  x=\"1115\" y=\"669\">Ankara</point>\n"
                               "    <point lat=\"42.697829\" lon=\"23.321657\"  x=\"920\"  y=\"656\">Sofia</point>\n"
                               "    <point lat=\"41.331645\" lon=\"19.831803\"  x=\"863\"  y=\"706\">Tirana</point>\n"
                               "    <point lat=\"43.856254\" lon=\"18.413086\"  x=\"827\"  y=\"646\">Sarajevo</point>\n"
                               "    <point lat=\"44.820553\" lon=\"20.462229\"  x=\"860\"  y=\"616\">Belgrade</point>\n"
                               "    <point lat=\"41.892918\" lon=\"12.482529\"  x=\"725\"  y=\"706\">Rom</point>\n"
                               "    <point lat=\"45.813023\" lon=\"15.977898\"  x=\"780\"  y=\"604\">Zagreb</point>\n"
                               "    <point lat=\"46.947919\" lon=\"7.444611\"   x=\"635\"  y=\"581\">Bern</point>\n"
                               "    <point lat=\"48.208173\" lon=\"16.373813\"  x=\"780\"  y=\"546\">Wien</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\"  x=\"745\"  y=\"504\">Prag</point>\n"
                               "    <point lat=\"48.145888\" lon=\"17.107129\"  x=\"826\"  y=\"556\">Bratislava</point>\n"
                               "    <point lat=\"44.432493\" lon=\"26.10388\"   x=\"960\"  y=\"603\">Bucharest</point>\n"
                               "    <point lat=\"46.48458\"  lon=\"30.732601\"  x=\"1020\" y=\"532\">Odesa</point>\n"
                               "    <point lat=\"50.450099\" lon=\"30.52341\"   x=\"980\"  y=\"442\">Kiev</point>\n"
                               "    <point lat=\"55.755834\" lon=\"37.617316\"  x=\"1022\" y=\"290\">Moscow</point>\n"
                               "    <point lat=\"53.900002\" lon=\"27.566671\"  x=\"915\"  y=\"379\">Minsk</point>\n"
                               "    <point lat=\"52.229679\" lon=\"21.012232\"  x=\"835\"  y=\"440\">Warsaw</point>\n"
                               "    <point lat=\"56.949644\" lon=\"24.105184\"  x=\"850\"  y=\"325\">Riga</point>\n"
                               "    <point lat=\"60.173324\" lon=\"24.941025\"  x=\"844\"  y=\"252\">Helsinki</point>\n"
                               "    <point lat=\"59.328921\" lon=\"18.064914\"  x=\"769\"  y=\"286\">Stockholm</point>\n"
                               "    <point lat=\"59.91387\"  lon=\"10.752246\"  x=\"686\"  y=\"279\">Oslo</point>\n"
                               "    <point lat=\"55.676096\" lon=\"12.568338\"  x=\"711\"  y=\"373\">Kopenhagen</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"   x=\"726\"  y=\"448\">Berlin</point>\n"
                               "    <point lat=\"50.85035\"  lon=\"4.351723\"   x=\"593\"  y=\"485\">Bruessel</point>\n"
                               "    <point lat=\"48.856612\" lon=\"2.352211\"   x=\"557\"  y=\"528\">Paris</point>\n"
                               "    <point lat=\"51.511226\" lon=\"-0.11982\"   x=\"529\"  y=\"460\">London</point>\n"
                               "    <point lat=\"53.349808\" lon=\"-6.260319\"  x=\"454\"  y=\"398\">Dublin</point>\n"
                               "    <point lat=\"64.135345\" lon=\"-21.895194\" x=\"385\"  y=\"101\">Rejkjavik</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Satellite Central Europe HRV",
                               "http://www.flugwetter.de/scripts/getimg.php?src=sat/mete_std_12-12-1_ceu_{0}_sat.jpg",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_12-12-1_ceu_{0}_sat.jpg"),
                               "%y%m%d%H%M",300,0,322,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_12-12-1_ceu_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Satellite Central Europe HRV {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"11\">\n"
                               "    <point lat=\"43.60465\"  lon=\"1.444209\"  x=\"148\" y=\"575\">Toulouse</point>\n"
                               "    <point lat=\"45.188532\" lon=\"5.724521\"  x=\"285\" y=\"556\">Grenoble</point>\n"
                               "    <point lat=\"45.833614\" lon=\"1.26111\"   x=\"155\" y=\"485\">Limoges</point>\n"
                               "    <point lat=\"48.113477\" lon=\"-1.675715\" x=\"93\"  y=\"382\">Rennes</point>\n"
                               "    <point lat=\"48.856612\" lon=\"2.352211\"  x=\"200\" y=\"372\">Paris</point>\n"
                               "    <point lat=\"47.322054\" lon=\"5.04148\"   x=\"268\" y=\"442\">Dijon</point>\n"
                               "    <point lat=\"48.692055\" lon=\"6.184418\"  x=\"304\" y=\"375\">Nancy</point>\n"
                               "    <point lat=\"50.85035\"  lon=\"4.351723\"  x=\"262\" y=\"299\">Bruessel</point>\n"
                               "    <point lat=\"52.370215\" lon=\"4.89516\"   x=\"276\" y=\"244\">Amsterdam</point>\n"
                               "    <point lat=\"51.511226\" lon=\"-0.11982\"  x=\"156\" y=\"259\">London</point>\n"
                               "    <point lat=\"53.479325\" lon=\"-2.248485\" x=\"123\" y=\"173\">Manchester</point>\n"
                               "    <point lat=\"54.597279\" lon=\"-5.930128\" x=\"52\"  y=\"113\">Belfast</point>\n"
                               "    <point lat=\"55.864235\" lon=\"-4.251816\" x=\"100\" y=\"77\">Glasgow</point>\n"
                               "    <point lat=\"57.048819\" lon=\"9.921749\"  x=\"399\" y=\"68\">Aalborg</point>\n"
                               "    <point lat=\"55.676096\" lon=\"12.568338\" x=\"455\" y=\"117\">Kopenhagen</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"400\" y=\"199\">Hamburg</point>\n"
                               "    <point lat=\"52.279908\" lon=\"8.047185\"  x=\"353\" y=\"249\">Osnabrück</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"393\" y=\"246\">Hannover</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"480\" y=\"241\">Berlin</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"326\" y=\"300\">Koeln</point>\n"
                               "    <point lat=\"51.312709\" lon=\"9.479742\"  x=\"387\" y=\"288\">Kassel</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"459\" y=\"286\">Leipzig</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"365\" y=\"337\">Frankfurt</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"507\" y=\"332\">Prag</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"380\" y=\"390\">Stuttgart</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"427\" y=\"359\">Nuernberg</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"400\" y=\"410\">Muenchen</point>\n"
                               "    <point lat=\"47.368652\" lon=\"8.539188\"  x=\"361\" y=\"441\">Zuerich</point>\n"
                               "    <point lat=\"45.465458\" lon=\"9.186523\"  x=\"378\" y=\"519\">Mailand</point>\n"
                               "    <point lat=\"46.198391\" lon=\"6.142291\"  x=\"293\" y=\"486\">Genf</point>\n"
                               "    <point lat=\"46.056452\" lon=\"14.508069\" x=\"525\" y=\"490\">Ljubljana</point>\n"
                               "    <point lat=\"45.813023\" lon=\"15.977898\" x=\"568\" y=\"498\">Zagreb</point>\n"
                               "    <point lat=\"48.306933\" lon=\"14.285831\" x=\"512\" y=\"401\">Linz</point>\n"
                               "    <point lat=\"48.208173\" lon=\"16.373813\" x=\"568\" y=\"400\">Wien</point>\n"
                               "    <point lat=\"48.145888\" lon=\"17.107129\" x=\"588\" y=\"399\">Bratislava</point>\n"
                               "    <point lat=\"47.497923\" lon=\"19.040251\" x=\"642\" y=\"419\">Budapest</point>\n"
                               "    <point lat=\"47.497923\" lon=\"19.040251\" x=\"715\" y=\"476\">Timisoara</point>\n"
                               "    <point lat=\"44.820553\" lon=\"20.462229\" x=\"701\" y=\"518\">Belgrade</point>\n"
                               "    <point lat=\"43.856254\" lon=\"18.413086\" x=\"648\" y=\"565\">Sarajevo</point>\n"
                               "    <point lat=\"52.406372\" lon=\"16.925168\" x=\"560\" y=\"235\">Poznan</point>\n"
                               "    <point lat=\"52.229679\" lon=\"21.012232\" x=\"659\" y=\"227\">Warsaw</point>\n"
                               "    <point lat=\"54.687154\" lon=\"25.279648\" x=\"733\" y=\"114\">Vilnius</point>\n"
                               "    <point lat=\"56.949644\" lon=\"24.105184\" x=\"688\" y=\"40\">Riga</point>\n"
                               "    <point lat=\"53.900002\" lon=\"27.566671\" x=\"788\" y=\"130\">Minsk</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Satellite Central Europe RGB",
                               "http://www.flugwetter.de/scripts/getimg.php?src=sat/mete_std_1-12-9i_ceu_{0}_sat.jpg",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_1-12-9i_ceu_{0}_sat.jpg"),
                               "%y%m%d%H%M",300,0,431,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_1-12-9i_ceu_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Satellite Central Europe RGB {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"11\">\n"
                               "    <point lat=\"43.60465\"  lon=\"1.444209\"  x=\"148\" y=\"575\">Toulouse</point>\n"
                               "    <point lat=\"45.188532\" lon=\"5.724521\"  x=\"285\" y=\"556\">Grenoble</point>\n"
                               "    <point lat=\"45.833614\" lon=\"1.26111\"   x=\"155\" y=\"485\">Limoges</point>\n"
                               "    <point lat=\"48.113477\" lon=\"-1.675715\" x=\"93\"  y=\"382\">Rennes</point>\n"
                               "    <point lat=\"48.856612\" lon=\"2.352211\"  x=\"200\" y=\"372\">Paris</point>\n"
                               "    <point lat=\"47.322054\" lon=\"5.04148\"   x=\"268\" y=\"442\">Dijon</point>\n"
                               "    <point lat=\"48.692055\" lon=\"6.184418\"  x=\"304\" y=\"375\">Nancy</point>\n"
                               "    <point lat=\"50.85035\"  lon=\"4.351723\"  x=\"262\" y=\"299\">Bruessel</point>\n"
                               "    <point lat=\"52.370215\" lon=\"4.89516\"   x=\"276\" y=\"244\">Amsterdam</point>\n"
                               "    <point lat=\"51.511226\" lon=\"-0.11982\"  x=\"156\" y=\"259\">London</point>\n"
                               "    <point lat=\"53.479325\" lon=\"-2.248485\" x=\"123\" y=\"173\">Manchester</point>\n"
                               "    <point lat=\"54.597279\" lon=\"-5.930128\" x=\"52\"  y=\"113\">Belfast</point>\n"
                               "    <point lat=\"55.864235\" lon=\"-4.251816\" x=\"100\" y=\"77\">Glasgow</point>\n"
                               "    <point lat=\"57.048819\" lon=\"9.921749\"  x=\"399\" y=\"68\">Aalborg</point>\n"
                               "    <point lat=\"55.676096\" lon=\"12.568338\" x=\"455\" y=\"117\">Kopenhagen</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"400\" y=\"199\">Hamburg</point>\n"
                               "    <point lat=\"52.279908\" lon=\"8.047185\"  x=\"353\" y=\"249\">Osnabrück</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"393\" y=\"246\">Hannover</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"480\" y=\"241\">Berlin</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"326\" y=\"300\">Koeln</point>\n"
                               "    <point lat=\"51.312709\" lon=\"9.479742\"  x=\"387\" y=\"288\">Kassel</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"459\" y=\"286\">Leipzig</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"365\" y=\"337\">Frankfurt</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"507\" y=\"332\">Prag</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"380\" y=\"390\">Stuttgart</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"427\" y=\"359\">Nuernberg</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"400\" y=\"410\">Muenchen</point>\n"
                               "    <point lat=\"47.368652\" lon=\"8.539188\"  x=\"361\" y=\"441\">Zuerich</point>\n"
                               "    <point lat=\"45.465458\" lon=\"9.186523\"  x=\"378\" y=\"519\">Mailand</point>\n"
                               "    <point lat=\"46.198391\" lon=\"6.142291\"  x=\"293\" y=\"486\">Genf</point>\n"
                               "    <point lat=\"46.056452\" lon=\"14.508069\" x=\"525\" y=\"490\">Ljubljana</point>\n"
                               "    <point lat=\"45.813023\" lon=\"15.977898\" x=\"568\" y=\"498\">Zagreb</point>\n"
                               "    <point lat=\"48.306933\" lon=\"14.285831\" x=\"512\" y=\"401\">Linz</point>\n"
                               "    <point lat=\"48.208173\" lon=\"16.373813\" x=\"568\" y=\"400\">Wien</point>\n"
                               "    <point lat=\"48.145888\" lon=\"17.107129\" x=\"588\" y=\"399\">Bratislava</point>\n"
                               "    <point lat=\"47.497923\" lon=\"19.040251\" x=\"642\" y=\"419\">Budapest</point>\n"
                               "    <point lat=\"47.497923\" lon=\"19.040251\" x=\"715\" y=\"476\">Timisoara</point>\n"
                               "    <point lat=\"44.820553\" lon=\"20.462229\" x=\"701\" y=\"518\">Belgrade</point>\n"
                               "    <point lat=\"43.856254\" lon=\"18.413086\" x=\"648\" y=\"565\">Sarajevo</point>\n"
                               "    <point lat=\"52.406372\" lon=\"16.925168\" x=\"560\" y=\"235\">Poznan</point>\n"
                               "    <point lat=\"52.229679\" lon=\"21.012232\" x=\"659\" y=\"227\">Warsaw</point>\n"
                               "    <point lat=\"54.687154\" lon=\"25.279648\" x=\"733\" y=\"114\">Vilnius</point>\n"
                               "    <point lat=\"56.949644\" lon=\"24.105184\" x=\"688\" y=\"40\">Riga</point>\n"
                               "    <point lat=\"53.900002\" lon=\"27.566671\" x=\"788\" y=\"130\">Minsk</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Satellite Alps HRV",
                               "http://www.flugwetter.de/scripts/getimg.php?src=sat/mete_std_12-12-1_alpen_{0}_sat.jpg",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_12-12-1_alpen_{0}_sat.jpg"),
                               "%y%m%d%H%M",300,0,227,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_12-12-1_alpen_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Satellite Alps HRV {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"11\">\n"
                               "    <point lat=\"48.692055\" lon=\"6.184418\"  x=\"178\" y=\"57\">Nancy</point>\n"
                               "    <point lat=\"47.322054\" lon=\"5.04148\"   x=\"106\" y=\"186\">Dijon</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"331\" y=\"86\">Stuttgart</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"425\" y=\"22\">Nuernberg</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"454\" y=\"125\">Muenchen</point>\n"
                               "    <point lat=\"48.306933\" lon=\"14.285831\" x=\"597\" y=\"108\">Linz</point>\n"
                               "    <point lat=\"48.208173\" lon=\"16.373813\" x=\"707\" y=\"105\">Wien</point>\n"
                               "    <point lat=\"48.145888\" lon=\"17.107129\" x=\"750\" y=\"104\">Bratislava</point>\n"
                               "    <point lat=\"47.368652\" lon=\"8.539188\"  x=\"293\" y=\"189\">Zuerich</point>\n"
                               "    <point lat=\"45.465458\" lon=\"9.186523\"  x=\"327\" y=\"343\">Mailand</point>\n"
                               "    <point lat=\"46.056452\" lon=\"14.508069\" x=\"622\" y=\"284\">Ljubljana</point>\n"
                               "    <point lat=\"45.813023\" lon=\"15.977898\" x=\"707\" y=\"298\">Zagreb</point>\n"
                               "    <point lat=\"46.198391\" lon=\"6.142291\"  x=\"158\" y=\"275\">Genf</point>\n"
                               "    <point lat=\"45.188532\" lon=\"5.724521\"  x=\"142\" y=\"417\">Grenoble</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Satellite Alps RGB",
                               "http://www.flugwetter.de/scripts/getimg.php?src=sat/mete_std_1-12-9i_alpen_{0}_sat.jpg",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_1-12-9i_alpen_{0}_sat.jpg"),
                               "%y%m%d%H%M",300,0,393,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_1-12-9i_alpen_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Satellite Alps RGB {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"11\">\n"
                               "    <point lat=\"48.692055\" lon=\"6.184418\"  x=\"178\" y=\"57\">Nancy</point>\n"
                               "    <point lat=\"47.322054\" lon=\"5.04148\"   x=\"106\" y=\"186\">Dijon</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"331\" y=\"86\">Stuttgart</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"425\" y=\"22\">Nuernberg</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"454\" y=\"125\">Muenchen</point>\n"
                               "    <point lat=\"48.306933\" lon=\"14.285831\" x=\"597\" y=\"108\">Linz</point>\n"
                               "    <point lat=\"48.208173\" lon=\"16.373813\" x=\"707\" y=\"105\">Wien</point>\n"
                               "    <point lat=\"48.145888\" lon=\"17.107129\" x=\"750\" y=\"104\">Bratislava</point>\n"
                               "    <point lat=\"47.368652\" lon=\"8.539188\"  x=\"293\" y=\"189\">Zuerich</point>\n"
                               "    <point lat=\"45.465458\" lon=\"9.186523\"  x=\"327\" y=\"343\">Mailand</point>\n"
                               "    <point lat=\"46.056452\" lon=\"14.508069\" x=\"622\" y=\"284\">Ljubljana</point>\n"
                               "    <point lat=\"45.813023\" lon=\"15.977898\" x=\"707\" y=\"298\">Zagreb</point>\n"
                               "    <point lat=\"46.198391\" lon=\"6.142291\"  x=\"158\" y=\"275\">Genf</point>\n"
                               "    <point lat=\"45.188532\" lon=\"5.724521\"  x=\"142\" y=\"417\">Grenoble</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Satellite Germany North West HRV",
                               "http://www.flugwetter.de/scripts/getimg.php?src=sat/mete_std_12-12-1_deu_nw_{0}_sat.jpg",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_12-12-1_deu_nw_{0}_sat.jpg"),
                               "%y%m%d%H%M",300,0,276,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_12-12-1_deu_nw_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Satellite Germany North West HRV {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10\">\n"
                               "    <point lat=\"55.676096\" lon=\"12.568338\" x=\"802\" y=\"88\">Kopenhagen</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"637\" y=\"333\">Hamburg</point>\n"
                               "    <point lat=\"52.279908\" lon=\"8.047185\"  x=\"498\" y=\"480\">Osnabrück</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"620\" y=\"474\">Hannover</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"878\" y=\"456\">Berlin</point>\n"
                               "    <point lat=\"50.85035\"  lon=\"4.351723\"  x=\"222\" y=\"633\">Bruessel</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"415\" y=\"636\">Koeln</point>\n"
                               "    <point lat=\"51.312709\" lon=\"9.479742\"  x=\"602\" y=\"600\">Kassel</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"815\" y=\"591\">Leipzig</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"533\" y=\"744\">Frankfurt</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"959\" y=\"727\">Prag</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Satellite Germany North West RGB",
                               "http://www.flugwetter.de/scripts/getimg.php?src=sat/mete_std_1-12-9i_deu_nw_{0}_sat.jpg",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_1-12-9i_deu_nw_{0}_sat.jpg"),
                               "%y%m%d%H%M",300,0,543,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_1-12-9i_deu_nw_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Satellite Germany North West RGB {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10\">\n"
                               "    <point lat=\"55.676096\" lon=\"12.568338\" x=\"802\" y=\"88\">Kopenhagen</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"637\" y=\"333\">Hamburg</point>\n"
                               "    <point lat=\"52.279908\" lon=\"8.047185\"  x=\"498\" y=\"480\">Osnabrück</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"620\" y=\"474\">Hannover</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"878\" y=\"456\">Berlin</point>\n"
                               "    <point lat=\"50.85035\"  lon=\"4.351723\"  x=\"222\" y=\"633\">Bruessel</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"415\" y=\"636\">Koeln</point>\n"
                               "    <point lat=\"51.312709\" lon=\"9.479742\"  x=\"602\" y=\"600\">Kassel</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"815\" y=\"591\">Leipzig</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"533\" y=\"744\">Frankfurt</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"959\" y=\"727\">Prag</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Satellite Germany North East HRV",
                               "http://www.flugwetter.de/scripts/getimg.php?src=sat/mete_std_12-12-1_deu_ne_{0}_sat.jpg",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_12-12-1_deu_ne_{0}_sat.jpg"),
                               "%y%m%d%H%M",300,0,276,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_12-12-1_deu_ne_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Satellite Germany North East HRV {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"11\">\n"
                               "    <point lat=\"55.676096\" lon=\"12.568338\" x=\"456\" y=\"91\">Kopenhagen</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"291\" y=\"333\">Hamburg</point>\n"
                               "    <point lat=\"52.279908\" lon=\"8.047185\"  x=\"150\" y=\"484\">Osnabrück</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"274\" y=\"477\">Hannover</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"531\" y=\"459\">Berlin</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"67\"  y=\"640\">Koeln</point>\n"
                               "    <point lat=\"51.312709\" lon=\"9.479742\"  x=\"256\" y=\"602\">Kassel</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"467\" y=\"595\">Leipzig</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"187\" y=\"747\">Frankfurt</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"611\" y=\"731\">Prag</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Satellite Germany North East RGB",
                               "http://www.flugwetter.de/scripts/getimg.php?src=sat/mete_std_1-12-9i_deu_ne_{0}_sat.jpg",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_1-12-9i_deu_ne_{0}_sat.jpg"),
                               "%y%m%d%H%M",300,0,564,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_1-12-9i_deu_ne_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Satellite Germany North East RGB {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"11\">\n"
                               "    <point lat=\"55.676096\" lon=\"12.568338\" x=\"456\" y=\"91\">Kopenhagen</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"291\" y=\"333\">Hamburg</point>\n"
                               "    <point lat=\"52.279908\" lon=\"8.047185\"  x=\"150\" y=\"484\">Osnabrück</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"274\" y=\"477\">Hannover</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"531\" y=\"459\">Berlin</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"67\"  y=\"640\">Koeln</point>\n"
                               "    <point lat=\"51.312709\" lon=\"9.479742\"  x=\"256\" y=\"602\">Kassel</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"467\" y=\"595\">Leipzig</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"187\" y=\"747\">Frankfurt</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"611\" y=\"731\">Prag</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Satellite Germany South West HRV",
                               "http://www.flugwetter.de/scripts/getimg.php?src=sat/mete_std_12-12-1_deu_sw_{0}_sat.jpg",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_12-12-1_deu_sw_{0}_sat.jpg"),
                               "%y%m%d%H%M",300,0,278,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_12-12-1_deu_sw_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Satellite Germany South West HRV {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"8\">\n"
                               "    <point lat=\"50.85035\"  lon=\"4.351723\"  x=\"243\" y=\"48\">Bruessel</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"434\" y=\"51\">Koeln</point>\n"
                               "    <point lat=\"51.312709\" lon=\"9.479742\"  x=\"622\" y=\"15\">Kassel</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"554\" y=\"159\">Frankfurt</point>\n"
                               "    <point lat=\"48.856612\" lon=\"2.352211\"  x=\"61\"  y=\"267\">Paris</point>\n"
                               "    <point lat=\"48.692055\" lon=\"6.184418\"  x=\"369\" y=\"278\">Nancy</point>\n"
                               "    <point lat=\"47.322054\" lon=\"5.04148\"   x=\"260\" y=\"472\">Dijon</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"599\" y=\"321\">Stuttgart</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"740\" y=\"224\">Nuernberg</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"979\" y=\"143\">Prag</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"782\" y=\"380\">Muenchen</point>\n"
                               "    <point lat=\"47.368652\" lon=\"8.539188\"  x=\"541\" y=\"474\">Zuerich</point>\n"
                               "    <point lat=\"45.465458\" lon=\"9.186523\"  x=\"590\" y=\"707\">Mailand</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Satellite Germany South West RGB",
                               "http://www.flugwetter.de/scripts/getimg.php?src=sat/mete_std_1-12-9i_deu_sw_{0}_sat.jpg",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_1-12-9i_deu_sw_{0}_sat.jpg"),
                               "%y%m%d%H%M",300,0,613,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_1-12-9i_deu_sw_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Satellite Germany South West RGB {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"8\">\n"
                               "    <point lat=\"50.85035\"  lon=\"4.351723\"  x=\"243\" y=\"48\">Bruessel</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"434\" y=\"51\">Koeln</point>\n"
                               "    <point lat=\"51.312709\" lon=\"9.479742\"  x=\"622\" y=\"15\">Kassel</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"554\" y=\"159\">Frankfurt</point>\n"
                               "    <point lat=\"48.856612\" lon=\"2.352211\"  x=\"61\"  y=\"267\">Paris</point>\n"
                               "    <point lat=\"48.692055\" lon=\"6.184418\"  x=\"369\" y=\"278\">Nancy</point>\n"
                               "    <point lat=\"47.322054\" lon=\"5.04148\"   x=\"260\" y=\"472\">Dijon</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"599\" y=\"321\">Stuttgart</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"740\" y=\"224\">Nuernberg</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"979\" y=\"143\">Prag</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"782\" y=\"380\">Muenchen</point>\n"
                               "    <point lat=\"47.368652\" lon=\"8.539188\"  x=\"541\" y=\"474\">Zuerich</point>\n"
                               "    <point lat=\"45.465458\" lon=\"9.186523\"  x=\"590\" y=\"707\">Mailand</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Satellite Germany South East HRV",
                               "http://www.flugwetter.de/scripts/getimg.php?src=sat/mete_std_12-12-1_deu_se_{0}_sat.jpg",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_12-12-1_deu_se_{0}_sat.jpg"),
                               "%y%m%d%H%M",300,0,295,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_12-12-1_deu_se_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Satellite Germany South East HRV {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"12\">\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"36\"  y=\"56\">Koeln</point>\n"
                               "    <point lat=\"51.312709\" lon=\"9.479742\"  x=\"225\" y=\"18\">Kassel</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"155\" y=\"162\">Frankfurt</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"200\" y=\"325\">Stuttgart</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"342\" y=\"227\">Nuernberg</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"580\" y=\"146\">Prag</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"385\" y=\"385\">Muenchen</point>\n"
                               "    <point lat=\"48.306933\" lon=\"14.285831\" x=\"599\" y=\"358\">Linz</point>\n"
                               "    <point lat=\"48.208173\" lon=\"16.373813\" x=\"764\" y=\"352\">Wien</point>\n"
                               "    <point lat=\"48.145888\" lon=\"17.107129\" x=\"827\" y=\"350\">Bratislava</point>\n"
                               "    <point lat=\"47.368652\" lon=\"8.539188\"  x=\"143\" y=\"478\">Zuerich</point>\n"
                               "    <point lat=\"45.465458\" lon=\"9.186523\"  x=\"193\" y=\"710\">Mailand</point>\n"
                               "    <point lat=\"46.056452\" lon=\"14.508069\" x=\"637\" y=\"624\">Ljubljana</point>\n"
                               "    <point lat=\"45.813023\" lon=\"15.977898\" x=\"762\" y=\"643\">Zagreb</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Satellite Germany South East RGB",
                               "http://www.flugwetter.de/scripts/getimg.php?src=sat/mete_std_1-12-9i_deu_se_{0}_sat.jpg",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_1-12-9i_deu_se_{0}_sat.jpg"),
                               "%y%m%d%H%M",300,0,587,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/mete_std_1-12-9i_deu_se_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Satellite Germany South East RGB {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"12\">\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"36\"  y=\"56\">Koeln</point>\n"
                               "    <point lat=\"51.312709\" lon=\"9.479742\"  x=\"225\" y=\"18\">Kassel</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"155\" y=\"162\">Frankfurt</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"200\" y=\"325\">Stuttgart</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"342\" y=\"227\">Nuernberg</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"580\" y=\"146\">Prag</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"385\" y=\"385\">Muenchen</point>\n"
                               "    <point lat=\"48.306933\" lon=\"14.285831\" x=\"599\" y=\"358\">Linz</point>\n"
                               "    <point lat=\"48.208173\" lon=\"16.373813\" x=\"764\" y=\"352\">Wien</point>\n"
                               "    <point lat=\"48.145888\" lon=\"17.107129\" x=\"827\" y=\"350\">Bratislava</point>\n"
                               "    <point lat=\"47.368652\" lon=\"8.539188\"  x=\"143\" y=\"478\">Zuerich</point>\n"
                               "    <point lat=\"45.465458\" lon=\"9.186523\"  x=\"193\" y=\"710\">Mailand</point>\n"
                               "    <point lat=\"46.056452\" lon=\"14.508069\" x=\"637\" y=\"624\">Ljubljana</point>\n"
                               "    <point lat=\"45.813023\" lon=\"15.977898\" x=\"762\" y=\"643\">Zagreb</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Berlin",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/lb_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lb_{0}.png"),
                               "%y%m%d%H%M",300,0,320,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lb_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Berlin {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"13.5\">\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"343\" y=\"455\">Berlin</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"233\" y=\"690\">Leipzig</point>\n"
                               "    <point lat=\"51.756312\" lon=\"14.332867\" x=\"467\" y=\"591\">Cottbus</point>\n"
                               "    <point lat=\"52.130801\" lon=\"11.628878\" x=\"132\" y=\"538\">Magdeburg</point>\n"
                               "    <point lat=\"51.050405\" lon=\"13.737266\" x=\"400\" y=\"736\">Dresden</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Dresden",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/lm_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lm_{0}.png"),
                               "%y%m%d%H%M",300,0,333,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lm_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Dresden {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"13.5\">\n"
                               "    <point lat=\"50.827843\" lon=\"12.92136\"  x=\"238\" y=\"525\">Chemnitz</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"279\" y=\"190\">Berlin</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"168\" y=\"427\">Leipzig</point>\n"
                               "    <point lat=\"51.756312\" lon=\"14.332867\" x=\"401\" y=\"329\">Cottbus</point>\n"
                               "    <point lat=\"52.130801\" lon=\"11.628878\" x=\"68\"  y=\"274\">Magdeburg</point>\n"
                               "    <point lat=\"51.050405\" lon=\"13.737266\" x=\"355\" y=\"472\">Dresden</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Eisberg",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/ln_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_ln_{0}.png"),
                               "%y%m%d%H%M",300,0,336,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_ln_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Eisberg {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"12.0\">\n"
                               "    <point lat=\"50.827843\" lon=\"12.92136\"  x=\"395\" y=\"198\">Chemnitz</point>\n"
                               "    <point lat=\"48.371446\" lon=\"10.898244\" x=\"147\" y=\"698\">Augsburg</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"238\" y=\"741\">Muenchen</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"167\" y=\"481\">Nuernberg</point>\n"
                               "    <point lat=\"49.105567\" lon=\"12.009344\" x=\"305\" y=\"561\">Regensburg</point>\n"
                               "    <point lat=\"48.404292\" lon=\"9.979341\"  x=\"28\"  y=\"695\">Ulm</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Emden",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/lh_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lh_{0}.png"),
                               "%y%m%d%H%M",300,0,186,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lh_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Emden {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"7.5\">\n"
                               "    <point lat=\"52.279908\" lon=\"8.047185\"  x=\"459\" y=\"670\">Osnabrück</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"666\" y=\"651\">Hannover</point>\n"
                               "    <point lat=\"53.079306\" lon=\"8.80168\"   x=\"557\" y=\"513\">Bremen</point>\n"
                               "    <point lat=\"54.793745\" lon=\"9.446998\"  x=\"630\" y=\"180\">Flensburg</point>\n"
                               "    <point lat=\"53.359401\" lon=\"7.206001\"  x=\"366\" y=\"453\">Emden</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Essen",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/lc_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lc_{0}.png"),
                               "%y%m%d%H%M",300,0,319,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lc_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Essen {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"7\">\n"
                               "    <point lat=\"52.279908\" lon=\"8.047185\"  x=\"477\" y=\"296\">Osnabrück</point>\n"
                               "    <point lat=\"53.079306\" lon=\"8.80168\"   x=\"574\" y=\"142\">Bremen</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"330\" y=\"549\">Koeln</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"536\" y=\"718\">Frankfurt</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Feldberg",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/ll_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_ll_{0}.png"),
                               "%y%m%d%H%M",300,0,616,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_ll_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Feldberg {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"8\">\n"
                               "    <point lat=\"48.404292\" lon=\"9.979341\"  x=\"614\" y=\"360\">Ulm</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"508\" y=\"279\">Stuttgart</point>\n"
                               "    <point lat=\"47.557422\" lon=\"7.592572\"  x=\"285\" y=\"521\">Basel</point>\n"
                               "    <point lat=\"47.999011\" lon=\"7.842114\"  x=\"322\" y=\"432\">Freiburg</point>\n"
                               "    <point lat=\"47.140191\" lon=\"7.243778\"  x=\"234\" y=\"604\"></point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Flechtdorf",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/lo_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lo_{0}.png"),
                               "%y%m%d%H%M",300,0,349,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lo_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Flechtdorf {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"8.5\">\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"108\" y=\"519\">Koeln</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"318\" y=\"690\">Frankfurt</point>\n"
                               "    <point lat=\"52.279908\" lon=\"8.047185\"  x=\"255\" y=\"260\">Osnabrück</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"462\" y=\"243\">Hannover</point>\n"
                               "    <point lat=\"50.984762\" lon=\"11.029887\" x=\"618\" y=\"518\">Erfurt</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Hamburg",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/le_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_le_{0}.png"),
                               "%y%m%d%H%M",300,0,246,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_le_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Hamburg {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10.5\">\n"
                               "    <point lat=\"52.279908\" lon=\"8.047185\"  x=\"108\" y=\"717\">Osnabrück</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"313\" y=\"699\">Hannover</point>\n"
                               "    <point lat=\"53.079306\" lon=\"8.80168\"   x=\"202\" y=\"561\">Bremen</point>\n"
                               "    <point lat=\"54.793745\" lon=\"9.446998\"  x=\"278\" y=\"230\">Flensburg</point>\n"
                               "    <point lat=\"52.130801\" lon=\"11.628878\" x=\"536\" y=\"749\">Magdeburg</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"342\" y=\"467\">Hamburg</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Hannover",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/la_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_la_{0}.png"),
                               "%y%m%d%H%M",300,0,313,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_la_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Hannover {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10\">\n"
                               "    <point lat=\"52.279908\" lon=\"8.047185\"  x=\"140\" y=\"487\">Osnabrück</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"349\" y=\"470\">Hannover</point>\n"
                               "    <point lat=\"53.079306\" lon=\"8.80168\"   x=\"238\" y=\"331\">Bremen</point>\n"
                               "    <point lat=\"52.130801\" lon=\"11.628878\" x=\"573\" y=\"518\">Magdeburg</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"377\" y=\"237\">Hamburg</point>\n"
                               "    <point lat=\"50.984762\" lon=\"11.029887\" x=\"505\" y=\"746\">Erfurt</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Memmingen",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/li_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_li_{0}.png"),
                               "%y%m%d%H%M",300,0,347,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_li_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Memmingen {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10.5\">\n"
                               "    <point lat=\"48.371446\" lon=\"10.898244\" x=\"433\" y=\"389\">Augsburg</point>\n"
                               "    <point lat=\"47.368652\" lon=\"8.539188\"  x=\"112\" y=\"585\">Zuerich</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"207\" y=\"305\">Stuttgart</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"450\" y=\"174\">Nuernberg</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"521\" y=\"431\">Muenchen</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Muenchen",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/lf_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lf_{0}.png"),
                               "%y%m%d%H%M",300,0,289,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lf_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Muenchen {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"11.5\">\n"
                               "    <point lat=\"48.371446\" lon=\"10.898244\" x=\"246\" y=\"451\">Augsburg</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"337\" y=\"495\">Muenchen</point>\n"
                               "    <point lat=\"48.404292\" lon=\"9.979341\"  x=\"126\" y=\"450\">Ulm</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"262\" y=\"237\">Nuernberg</point>\n"
                               "    <point lat=\"49.105567\" lon=\"12.009344\" x=\"403\" y=\"317\">Regensburg</point>\n"
                               "    <point lat=\"47.809494\" lon=\"13.055019\" x=\"542\" y=\"557\">Salzburg</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Neuhaus",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/lg_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lg_{0}.png"),
                               "%y%m%d%H%M",300,0,352,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lg_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Neuhaus {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"11\">\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"332\" y=\"667\">Nuernberg</point>\n"
                               "    <point lat=\"50.827843\" lon=\"12.92136\"  x=\"563\" y=\"386\">Chemnitz</point>\n"
                               "    <point lat=\"50.984762\" lon=\"11.029887\" x=\"324\" y=\"360\">Erfurt</point>\n"
                               "    <point lat=\"51.312709\" lon=\"9.479742\"  x=\"133\" y=\"296\">Kassel</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Wideumont",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/lv_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lv_{0}.png"),
                               "%y%m%d%H%M",300,0,355,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lv_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Wideumont {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"6\">\n"
                               "    <point lat=\"49.240158\" lon=\"6.996939\"  x=\"515\" y=\"602\">Saarbruecken</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"542\" y=\"260\">Koeln</point>\n"
                               "    <point lat=\"N49\" lon=\"E4\" x=\"129\" y=\"621\"></point>\n"
                               "    <point lat=\"N49\" lon=\"E8\" x=\"655\" y=\"658\"></point>\n"
                               "    <point lat=\"N51\" lon=\"E4\" x=\"171\" y=\"224\"></point>\n"
                               "    <point lat=\"N51\" lon=\"E8\" x=\"669\" y=\"259\"></point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Offenthal",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/lt_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lt_{0}.png"),
                               "%y%m%d%H%M",300,0,357,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lt_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Offenthal {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"9\">\n"
                               "    <point lat=\"51.312709\" lon=\"9.479742\"  x=\"445\" y=\"196\">Kassel</point>\n"
                               "    <point lat=\"49.240158\" lon=\"6.996939\"  x=\"106\" y=\"594\">Saarbruecken</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"129\" y=\"255\">Koeln</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"644\" y=\"563\">Nuernberg</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"401\" y=\"696\">Stuttgart</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"339\" y=\"428\">Frankfurt</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Rostock",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/lj_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lj_{0}.png"),
                               "%y%m%d%H%M",300,0,239,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lj_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Rostock {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"12\">\n"
                               "    <point lat=\"54.793745\" lon=\"9.446998\"  x=\"42\"  y=\"339\">Flensburg</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"107\" y=\"577\">Hamburg</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"516\" y=\"777\">Berlin</point>\n"
                               "    <point lat=\"54.29941\"  lon=\"13.087721\" x=\"459\" y=\"426\">Stralsund</point>\n"
                               "    <point lat=\"54.083424\" lon=\"12.100432\" x=\"345\" y=\"470\">Rostock</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDB",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/rb_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rb_{0}.png"),
                               "%y%m%d%H%M",300,0,738,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rb_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDB {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"14\">\n"
                               "    <north tailx=\"666\" taily=\"296\" headx=\"648\" heady=\"14\" dist=\"54\" />\n"
                               "    <point lat=\"N52 22.35367\" lon=\"E013 30.33369\" x=\"666\" y=\"296\">EDDB</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDC",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/rc_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rc_{0}.png"),
                               "%y%m%d%H%M",300,0,788,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rc_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDC {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"14\">\n"
                               "    <north tailx=\"682\" taily=\"322\" headx=\"664\" heady=\"39\" dist=\"54\" />\n"
                               "    <point lat=\"N51 08.06206\" lon=\"E013 46.08391\" x=\"682\" y=\"322\">EDDC</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDE",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/re_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_re_{0}.png"),
                               "%y%m%d%H%M",300,0,854,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_re_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDE {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"11\">\n"
                               "    <north tailx=\"499\" taily=\"329\" headx=\"494\" heady=\"44\" dist=\"54\" />\n"
                               "    <point lat=\"N50 58.78848\" lon=\"E010 57.48634\" x=\"499\" y=\"329\">EDDE</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDF",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/rf_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rf_{0}.png"),
                               "%y%m%d%H%M",300,0,911,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rf_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDF {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"9\">\n"
                               "    <north tailx=\"561\" taily=\"355\" headx=\"568\" heady=\"69\" dist=\"54\" />\n"
                               "    <point lat=\"N50 02.02789\" lon=\"E008 33.62117\" x=\"561\" y=\"355\">EDDF</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDG",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/rg_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rg_{0}.png"),
                               "%y%m%d%H%M",300,0,755,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rg_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDG {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"8\">\n"
                               "    <north tailx=\"545\" taily=\"350\" headx=\"555\" heady=\"68\" dist=\"54\" />\n"
                               "    <point lat=\"N52 08.07493\" lon=\"E007 41.08014\" x=\"545\" y=\"350\">EDDG</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDH",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/rh_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rh_{0}.png"),
                               "%y%m%d%H%M",300,0,608,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rh_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDH {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10\">\n"
                               "    <north tailx=\"472\" taily=\"366\" headx=\"471\" heady=\"87\" dist=\"54\" />\n"
                               "    <point lat=\"N53 37.65432\" lon=\"E009 58.95666\" x=\"472\" y=\"366\">EDDH</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDK",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/rk_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rk_{0}.png"),
                               "%y%m%d%H%M",300,0,885,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rk_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDK {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"7\">\n"
                               "    <north tailx=\"500\" taily=\"344\" headx=\"514\" heady=\"61\" dist=\"54\" />\n"
                               "    <point lat=\"N50 51.89381\" lon=\"E007 08.37150\" x=\"500\" y=\"344\">EDDK</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDL",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/rl_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rl_{0}.png"),
                               "%y%m%d%H%M",300,0,829,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rl_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDL {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"7\">\n"
                               "    <north tailx=\"496\" taily=\"359\" headx=\"512\" heady=\"76\" dist=\"54\" />\n"
                               "    <point lat=\"N51 17.26772\" lon=\"E006 46.14994\" x=\"496\" y=\"359\">EDDL</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDM",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/rm_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rm_{0}.png"),
                               "%y%m%d%H%M",300,0,804,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rm_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDM {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"12\">\n"
                               "    <north tailx=\"591\" taily=\"323\" headx=\"583\" heady=\"35\" dist=\"54\" />\n"
                               "    <point lat=\"N48 20.56508\" lon=\"E011 46.67237\" x=\"591\" y=\"323\">EDDM</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDN",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/rn_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rn_{0}.png"),
                               "%y%m%d%H%M",300,0,825,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rn_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDN {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"11\">\n"
                               "    <north tailx=\"534\" taily=\"313\" headx=\"529\" heady=\"25\" dist=\"54\" />\n"
                               "    <point lat=\"N49 29.92459\" lon=\"E011 04.65479\" x=\"534\" y=\"313\">EDDN</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDP",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/rp_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rp_{0}.png"),
                               "%y%m%d%H%M",300,0,772,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rp_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDP {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"12\">\n"
                               "    <north tailx=\"427\" taily=\"374\" headx=\"416\" heady=\"92\" dist=\"54\" />\n"
                               "    <point lat=\"N51 25.94681\" lon=\"E012 14.49809\" x=\"427\" y=\"374\">EDDP</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDR",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/rr_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rr_{0}.png"),
                               "%y%m%d%H%M",300,0,909,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rr_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDR {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"7\">\n"
                               "    <north tailx=\"476\" taily=\"368\" headx=\"490\" heady=\"82\" dist=\"54\" />\n"
                               "    <point lat=\"N49 12.87427\" lon=\"E007 06.59642\" x=\"476\" y=\"368\">EDDR</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDS",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/rs_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rs_{0}.png"),
                               "%y%m%d%H%M",300,0,842,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rs_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDS {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"9\">\n"
                               "    <north tailx=\"456\" taily=\"315\" headx=\"461\" heady=\"27\" dist=\"54\" />\n"
                               "    <point lat=\"N48 41.39264\" lon=\"E009 13.31794\" x=\"456\" y=\"315\">EDDS</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDT",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/rt_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rt_{0}.png"),
                               "%y%m%d%H%M",300,0,744,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rt_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDT {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"13\">\n"
                               "    <north tailx=\"622\" taily=\"355\" headx=\"606\" heady=\"74\" dist=\"54\" />\n"
                               "    <point lat=\"N52 33.50436\" lon=\"E013 17.55721\" x=\"622\" y=\"355\">EDDT</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDV",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/rv_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rv_{0}.png"),
                               "%y%m%d%H%M",300,0,767,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rv_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDV {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10\">\n"
                               "    <north tailx=\"515\" taily=\"383\" headx=\"517\" heady=\"102\" dist=\"54\" />\n"
                               "    <point lat=\"N52 27.87076\" lon=\"E009 41.38246\" x=\"515\" y=\"383\">EDDV</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDW",
                               "http://www.flugwetter.de/scripts/getimg.php?src=rad/rw_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rw_{0}.png"),
                               "%y%m%d%H%M",300,0,652,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rw_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDW {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"9\">\n"
                               "    <north tailx=\"543\" taily=\"363\" headx=\"548\" heady=\"83\" dist=\"54\" />\n"
                               "    <point lat=\"N53 02.67258\" lon=\"E008 47.29919\" x=\"543\" y=\"363\">EDDW</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
        if passwd & PASSWD_FLUGWETTERIMGCOMPR:
            self.store.append(["Radar Europa (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/pm_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_pm_{0}.jpg"),
                               "%y%m%d%H%M",300,0,147,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_pm_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Europe {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"5\">\n"
                               "    <point lat=\"45.465458\" lon=\"9.186523\"  x=\"448\" y=\"598\">Mailand</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"519\" y=\"480\">Muenchen</point>\n"
                               "    <point lat=\"48.208173\" lon=\"16.373813\" x=\"660\" y=\"468\">Wien</point>\n"
                               "    <point lat=\"48.856612\" lon=\"2.352211\"  x=\"254\" y=\"434\">Paris</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"437\" y=\"392\">Frankfurt</point>\n"
                               "    <point lat=\"50.85035\"  lon=\"4.351723\"  x=\"320\" y=\"355\">Bruessel</point>\n"
                               "    <point lat=\"52.370215\" lon=\"4.89516\"   x=\"340\" y=\"290\">Amsterdam</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"562\" y=\"288\">Berlin</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"474\" y=\"245\">Hamburg</point>\n"
                               "    <point lat=\"55.676096\" lon=\"12.568338\" x=\"533\" y=\"155\">Kopenhagen</point>\n"
                               "    <point lat=\"51.511226\" lon=\"-0.11982\"  x=\"203\" y=\"310\">London</point>\n"
                               "    <point lat=\"53.349808\" lon=\"-6.260319\" x=\"64\"  y=\"196\">Dublin</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Alpen (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/r5_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_r5_{0}.jpg"),
                               "%y%m%d%H%M",300,0,245,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_r5_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Alpen {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10\">\n"
                               "    <point x=\"61\"  y=\"643\" lat=\"43.433584571\" lon=\"5.213687606\">LFML</point>\n"
                               "    <point x=\"225\" y=\"632\" lat=\"43.656067103\" lon=\"7.215508707\">LFMN</point>\n"
                               "    <point x=\"87\"  y=\"426\" lat=\"45.362728536\" lon=\"5.329292323\">LFLS</point>\n"
                               "    <point x=\"132\" y=\"400\" lat=\"45.637590587\" lon=\"5.880322829\">LFLB</point>\n"
                               "    <point x=\"70\"  y=\"384\" lat=\"45.728640854\" lon=\"5.088146813\">LFLL</point>\n"
                               "    <point x=\"270\" y=\"454\" lat=\"45.200763345\" lon=\"7.649630606\">LIMF</point>\n"
                               "    <point x=\"363\" y=\"548\" lat=\"44.411957860\" lon=\"8.841866143\">LIMJ</point>\n"
                               "    <point x=\"357\" y=\"412\" lat=\"45.628117025\" lon=\"8.723268248\">LIMC</point>\n"
                               "    <point x=\"157\" y=\"332\" lat=\"46.238094270\" lon=\"6.109000593\">LSGG</point>\n"
                               "    <point x=\"85\"  y=\"212\" lat=\"47.272201180\" lon=\"5.085252710\">LFSD</point>\n"
                               "    <point x=\"153\" y=\"220\" lat=\"47.274338901\" lon=\"6.084167361\">LFSA</point>\n"
                               "    <point x=\"271\" y=\"187\" lat=\"47.589798868\" lon=\"7.528706230\">LFSB</point>\n"
                               "    <point x=\"266\" y=\"263\" lat=\"46.913436949\" lon=\"7.497960068\">LSZB</point>\n"
                               "    <point x=\"346\" y=\"203\" lat=\"47.457772493\" lon=\"8.553960845\">LSZH</point>\n"
                               "    <point x=\"419\" y=\"186\" lat=\"47.671316564\" lon=\"9.511491694\">EDNY</point>\n"
                               "    <point x=\"401\" y=\"71\"  lat=\"48.689877391\" lon=\"9.221965671\">EDDS</point>\n"
                               "    <point x=\"556\" y=\"618\" lat=\"43.809984326\" lon=\"11.205117591\">LIRQ</point>\n"
                               "    <point x=\"562\" y=\"535\" lat=\"44.535446763\" lon=\"11.288662702\">LIPE</point>\n"
                               "    <point x=\"641\" y=\"423\" lat=\"45.505153835\" lon=\"12.351905778\">LIPZ</point>\n"
                               "    <point x=\"771\" y=\"484\" lat=\"44.893533736\" lon=\"13.922155425\">LDPL</point>\n"
                               "    <point x=\"894\" y=\"565\" lat=\"44.086080194\" lon=\"15.349408463\">LDZD</point>\n"
                               "    <point x=\"933\" y=\"375\" lat=\"45.741893649\" lon=\"16.067201123\">LDZA</point>\n"
                               "    <point x=\"705\" y=\"360\" lat=\"46.031397879\" lon=\"13.186903596\">LIPD</point>\n"
                               "    <point x=\"804\" y=\"332\" lat=\"46.223685443\" lon=\"14.457608834\">LJLJ</point>\n"
                               "    <point x=\"789\" y=\"285\" lat=\"46.643188298\" lon=\"14.324599430\">LOWK</point>\n"
                               "    <point x=\"870\" y=\"239\" lat=\"46.991068125\" lon=\"15.439625904\">LOWG</point>\n"
                               "    <point x=\"559\" y=\"227\" lat=\"47.260225117\" lon=\"11.343967170\">LOWI</point>\n"
                               "    <point x=\"681\" y=\"163\" lat=\"47.793332934\" lon=\"13.004333675\">LOWS</point>\n"
                               "    <point x=\"765\" y=\"110\" lat=\"48.233220577\" lon=\"14.187513739\">LOWL</point>\n"
                               "    <point x=\"942\" y=\"106\" lat=\"48.115932941\" lon=\"16.554506868\">LOWW</point>\n"
                               "    <point x=\"589\" y=\"104\" lat=\"48.342751265\" lon=\"11.777872816\">EDDM</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Deutschland Süd (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/ri_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_ri_{0}.jpg"),
                               "%y%m%d%H%M",300,0,240,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_ri_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Deutschland Süd {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"11.5\">\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"684\" y=\"546\">Muenchen</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"401\" y=\"249\">Frankfurt</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"631\" y=\"352\">Nuernberg</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"951\" y=\"243\">Prag</point>\n"
                               "    <point lat=\"47.368652\" lon=\"8.539188\"  x=\"377\" y=\"662\">Zuerich</point>\n"
                               "    <point lat=\"49.240158\" lon=\"6.996939\"  x=\"233\" y=\"377\">Saarbruecken</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"444\" y=\"453\">Stuttgart</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"246\" y=\"120\">Koeln</point>\n"
                               "    <point lat=\"51.050405\" lon=\"13.737266\" x=\"873\" y=\"101\">Dresden</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"747\" y=\"67\">Leipzig</point>\n"
                               "    <point lat=\"47.557422\" lon=\"7.592572\"  x=\"282\" y=\"631\">Basel</point>\n"
                               "    <point lat=\"47.809494\" lon=\"13.055019\"  x=\"835\" y=\"592\">Salzburg</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Deutschland Mitte (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/rd_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rd_{0}.jpg"),
                               "%y%m%d%H%M",300,0,224,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rd_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Deutschland Mitte {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10.5\">\n"
                               "    <point lat=\"48.371446\" lon=\"10.898244\" x=\"617\" y=\"687\">Augsburg</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"401\" y=\"421\">Frankfurt</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"629\" y=\"522\">Nuernberg</point>\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"950\" y=\"414\">Prag</point>\n"
                               "    <point lat=\"49.240158\" lon=\"6.996939\"  x=\"233\" y=\"549\">Saarbruecken</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"448\" y=\"624\">Stuttgart</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"246\" y=\"296\">Koeln</point>\n"
                               "    <point lat=\"51.050405\" lon=\"13.737266\" x=\"874\" y=\"273\">Dresden</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"747\" y=\"238\">Leipzig</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"832\" y=\"62\">Berlin</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"502\" y=\"88\">Hannover</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Deutschland Nord (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/ra_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_ra_{0}.jpg"),
                               "%y%m%d%H%M",300,0,191,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_ra_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Deutschland Nord {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10.0\">\n"
                               "    <point lat=\"50.07554\"  lon=\"14.437795\" x=\"951\" y=\"697\">Prag</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"246\" y=\"572\">Koeln</point>\n"
                               "    <point lat=\"51.050405\" lon=\"13.737266\" x=\"874\" y=\"555\">Dresden</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"747\" y=\"518\">Leipzig</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"830\" y=\"341\">Berlin</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"504\" y=\"368\">Hannover</point>\n"
                               "    <point lat=\"50.360028\" lon=\"7.5899\"    x=\"291\" y=\"651\">Koblenz</point>\n"
                               "    <point lat=\"53.079306\" lon=\"8.80168\"   x=\"417\" y=\"265\">Bremen</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"528\" y=\"195\">Hamburg</point>\n"
                               "    <point lat=\"54.323294\" lon=\"10.122764\" x=\"539\" y=\"86\">Kiel</point>\n"
                               "    <point lat=\"54.793745\" lon=\"9.446998\"  x=\"481\" y=\"15\">Flensburg</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Berlin (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/lb_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lb_{0}.jpg"),
                               "%y%m%d%H%M",300,0,164,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lb_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Berlin {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"13.5\">\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"343\" y=\"455\">Berlin</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"233\" y=\"690\">Leipzig</point>\n"
                               "    <point lat=\"51.756312\" lon=\"14.332867\" x=\"467\" y=\"591\">Cottbus</point>\n"
                               "    <point lat=\"52.130801\" lon=\"11.628878\" x=\"132\" y=\"538\">Magdeburg</point>\n"
                               "    <point lat=\"51.050405\" lon=\"13.737266\" x=\"400\" y=\"736\">Dresden</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Dresden (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/lm_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lm_{0}.jpg"),
                               "%y%m%d%H%M",300,0,161,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lm_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Dresden {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"13.5\">\n"
                               "    <point lat=\"50.827843\" lon=\"12.92136\"  x=\"238\" y=\"525\">Chemnitz</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"279\" y=\"190\">Berlin</point>\n"
                               "    <point lat=\"51.339701\" lon=\"12.373073\" x=\"168\" y=\"427\">Leipzig</point>\n"
                               "    <point lat=\"51.756312\" lon=\"14.332867\" x=\"401\" y=\"329\">Cottbus</point>\n"
                               "    <point lat=\"52.130801\" lon=\"11.628878\" x=\"68\"  y=\"274\">Magdeburg</point>\n"
                               "    <point lat=\"51.050405\" lon=\"13.737266\" x=\"355\" y=\"472\">Dresden</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Eisberg (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/ln_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_ln_{0}.jpg"),
                               "%y%m%d%H%M",300,0,167,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_ln_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Eisberg {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"12.0\">\n"
                               "    <point lat=\"50.827843\" lon=\"12.92136\"  x=\"395\" y=\"198\">Chemnitz</point>\n"
                               "    <point lat=\"48.371446\" lon=\"10.898244\" x=\"147\" y=\"698\">Augsburg</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"238\" y=\"741\">Muenchen</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"167\" y=\"481\">Nuernberg</point>\n"
                               "    <point lat=\"49.105567\" lon=\"12.009344\" x=\"305\" y=\"561\">Regensburg</point>\n"
                               "    <point lat=\"48.404292\" lon=\"9.979341\"  x=\"28\"  y=\"695\">Ulm</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Emden (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/lh_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lh_{0}.jpg"),
                               "%y%m%d%H%M",300,0,162,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lh_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Emden {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"7.5\">\n"
                               "    <point lat=\"52.279908\" lon=\"8.047185\"  x=\"459\" y=\"670\">Osnabrück</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"666\" y=\"651\">Hannover</point>\n"
                               "    <point lat=\"53.079306\" lon=\"8.80168\"   x=\"557\" y=\"513\">Bremen</point>\n"
                               "    <point lat=\"54.793745\" lon=\"9.446998\"  x=\"630\" y=\"180\">Flensburg</point>\n"
                               "    <point lat=\"53.359401\" lon=\"7.206001\"  x=\"366\" y=\"453\">Emden</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Essen (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/lc_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lc_{0}.jpg"),
                               "%y%m%d%H%M",300,0,168,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lc_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Essen {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"7\">\n"
                               "    <point lat=\"52.279908\" lon=\"8.047185\"  x=\"477\" y=\"296\">Osnabrück</point>\n"
                               "    <point lat=\"53.079306\" lon=\"8.80168\"   x=\"574\" y=\"142\">Bremen</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"330\" y=\"549\">Koeln</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"536\" y=\"718\">Frankfurt</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Feldberg (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/ll_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_ll_{0}.jpg"),
                               "%y%m%d%H%M",300,0,177,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_ll_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Feldberg {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"8\">\n"
                               "    <point lat=\"48.404292\" lon=\"9.979341\"  x=\"614\" y=\"360\">Ulm</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"508\" y=\"279\">Stuttgart</point>\n"
                               "    <point lat=\"47.557422\" lon=\"7.592572\"  x=\"285\" y=\"521\">Basel</point>\n"
                               "    <point lat=\"47.999011\" lon=\"7.842114\"  x=\"322\" y=\"432\">Freiburg</point>\n"
                               "    <point lat=\"47.140191\" lon=\"7.243778\"  x=\"234\" y=\"604\"></point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Flechtdorf (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/lo_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lo_{0}.jpg"),
                               "%y%m%d%H%M",300,0,169,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lo_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Flechtdorf {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"8.5\">\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"108\" y=\"519\">Koeln</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"318\" y=\"690\">Frankfurt</point>\n"
                               "    <point lat=\"52.279908\" lon=\"8.047185\"  x=\"255\" y=\"260\">Osnabrück</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"462\" y=\"243\">Hannover</point>\n"
                               "    <point lat=\"50.984762\" lon=\"11.029887\" x=\"618\" y=\"518\">Erfurt</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Hamburg (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/le_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_le_{0}.jpg"),
                               "%y%m%d%H%M",300,0,161,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_le_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Hamburg {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10.5\">\n"
                               "    <point lat=\"52.279908\" lon=\"8.047185\"  x=\"108\" y=\"717\">Osnabrück</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"313\" y=\"699\">Hannover</point>\n"
                               "    <point lat=\"53.079306\" lon=\"8.80168\"   x=\"202\" y=\"561\">Bremen</point>\n"
                               "    <point lat=\"54.793745\" lon=\"9.446998\"  x=\"278\" y=\"230\">Flensburg</point>\n"
                               "    <point lat=\"52.130801\" lon=\"11.628878\" x=\"536\" y=\"749\">Magdeburg</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"342\" y=\"467\">Hamburg</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Hannover (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/la_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_la_{0}.jpg"),
                               "%y%m%d%H%M",300,0,166,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_la_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Hannover {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10\">\n"
                               "    <point lat=\"52.279908\" lon=\"8.047185\"  x=\"140\" y=\"487\">Osnabrück</point>\n"
                               "    <point lat=\"52.375887\" lon=\"9.73202\"   x=\"349\" y=\"470\">Hannover</point>\n"
                               "    <point lat=\"53.079306\" lon=\"8.80168\"   x=\"238\" y=\"331\">Bremen</point>\n"
                               "    <point lat=\"52.130801\" lon=\"11.628878\" x=\"573\" y=\"518\">Magdeburg</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"377\" y=\"237\">Hamburg</point>\n"
                               "    <point lat=\"50.984762\" lon=\"11.029887\" x=\"505\" y=\"746\">Erfurt</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Memmingen (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/li_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_li_{0}.jpg"),
                               "%y%m%d%H%M",300,0,178,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_li_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Memmingen {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10.5\">\n"
                               "    <point lat=\"48.371446\" lon=\"10.898244\" x=\"433\" y=\"389\">Augsburg</point>\n"
                               "    <point lat=\"47.368652\" lon=\"8.539188\"  x=\"112\" y=\"585\">Zuerich</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"207\" y=\"305\">Stuttgart</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"450\" y=\"174\">Nuernberg</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"521\" y=\"431\">Muenchen</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Muenchen (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/lf_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lf_{0}.jpg"),
                               "%y%m%d%H%M",300,0,197,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lf_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Muenchen {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"11.5\">\n"
                               "    <point lat=\"48.371446\" lon=\"10.898244\" x=\"246\" y=\"451\">Augsburg</point>\n"
                               "    <point lat=\"48.136724\" lon=\"11.576757\" x=\"337\" y=\"495\">Muenchen</point>\n"
                               "    <point lat=\"48.404292\" lon=\"9.979341\"  x=\"126\" y=\"450\">Ulm</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"262\" y=\"237\">Nuernberg</point>\n"
                               "    <point lat=\"49.105567\" lon=\"12.009344\" x=\"403\" y=\"317\">Regensburg</point>\n"
                               "    <point lat=\"47.809494\" lon=\"13.055019\" x=\"542\" y=\"557\">Salzburg</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Neuhaus (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/lg_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lg_{0}.jpg"),
                               "%y%m%d%H%M",300,0,162,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lg_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Neuhaus {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"11\">\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"332\" y=\"667\">Nuernberg</point>\n"
                               "    <point lat=\"50.827843\" lon=\"12.92136\"  x=\"563\" y=\"386\">Chemnitz</point>\n"
                               "    <point lat=\"50.984762\" lon=\"11.029887\" x=\"324\" y=\"360\">Erfurt</point>\n"
                               "    <point lat=\"51.312709\" lon=\"9.479742\"  x=\"133\" y=\"296\">Kassel</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Wideumont (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/lv_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lv_{0}.jpg"),
                               "%y%m%d%H%M",300,0,177,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lv_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Wideumont {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"6\">\n"
                               "    <point lat=\"49.240158\" lon=\"6.996939\"  x=\"515\" y=\"602\">Saarbruecken</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"542\" y=\"260\">Koeln</point>\n"
                               "    <point lat=\"N49\" lon=\"E4\" x=\"129\" y=\"621\"></point>\n"
                               "    <point lat=\"N49\" lon=\"E8\" x=\"655\" y=\"658\"></point>\n"
                               "    <point lat=\"N51\" lon=\"E4\" x=\"171\" y=\"224\"></point>\n"
                               "    <point lat=\"N51\" lon=\"E8\" x=\"669\" y=\"259\"></point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Offenthal (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/lt_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lt_{0}.jpg"),
                               "%y%m%d%H%M",300,0,170,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lt_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Offenthal {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"9\">\n"
                               "    <point lat=\"51.312709\" lon=\"9.479742\"  x=\"445\" y=\"196\">Kassel</point>\n"
                               "    <point lat=\"49.240158\" lon=\"6.996939\"  x=\"106\" y=\"594\">Saarbruecken</point>\n"
                               "    <point lat=\"50.937527\" lon=\"6.960289\"  x=\"129\" y=\"255\">Koeln</point>\n"
                               "    <point lat=\"49.452029\" lon=\"11.076751\" x=\"644\" y=\"563\">Nuernberg</point>\n"
                               "    <point lat=\"48.775424\" lon=\"9.181759\"  x=\"401\" y=\"696\">Stuttgart</point>\n"
                               "    <point lat=\"50.110919\" lon=\"8.682117\"  x=\"339\" y=\"428\">Frankfurt</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar Rostock (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/lj_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lj_{0}.jpg"),
                               "%y%m%d%H%M",300,0,158,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_lj_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar Rostock {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"12\">\n"
                               "    <point lat=\"54.793745\" lon=\"9.446998\"  x=\"42\"  y=\"339\">Flensburg</point>\n"
                               "    <point lat=\"53.551094\" lon=\"9.993696\"  x=\"107\" y=\"577\">Hamburg</point>\n"
                               "    <point lat=\"52.519172\" lon=\"13.40611\"  x=\"516\" y=\"777\">Berlin</point>\n"
                               "    <point lat=\"54.29941\"  lon=\"13.087721\" x=\"459\" y=\"426\">Stralsund</point>\n"
                               "    <point lat=\"54.083424\" lon=\"12.100432\" x=\"345\" y=\"470\">Rostock</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDB (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/rb_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rb_{0}.jpg"),
                               "%y%m%d%H%M",300,0,165,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rb_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDB {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"14\">\n"
                               "    <north tailx=\"666\" taily=\"296\" headx=\"648\" heady=\"14\" dist=\"54\" />\n"
                               "    <point lat=\"N52 22.35367\" lon=\"E013 30.33369\" x=\"666\" y=\"296\">EDDB</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDC (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/rc_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rc_{0}.jpg"),
                               "%y%m%d%H%M",300,0,173,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rc_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDC {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"14\">\n"
                               "    <north tailx=\"682\" taily=\"322\" headx=\"664\" heady=\"39\" dist=\"54\" />\n"
                               "    <point lat=\"N51 08.06206\" lon=\"E013 46.08391\" x=\"682\" y=\"322\">EDDC</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDE (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/re_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_re_{0}.jpg"),
                               "%y%m%d%H%M",300,0,172,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_re_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDE {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"11\">\n"
                               "    <north tailx=\"499\" taily=\"329\" headx=\"494\" heady=\"44\" dist=\"54\" />\n"
                               "    <point lat=\"N50 58.78848\" lon=\"E010 57.48634\" x=\"499\" y=\"329\">EDDE</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDF (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/rf_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rf_{0}.jpg"),
                               "%y%m%d%H%M",300,0,191,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rf_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDF {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"9\">\n"
                               "    <north tailx=\"561\" taily=\"355\" headx=\"568\" heady=\"69\" dist=\"54\" />\n"
                               "    <point lat=\"N50 02.02789\" lon=\"E008 33.62117\" x=\"561\" y=\"355\">EDDF</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDG (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/rg_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rg_{0}.jpg"),
                               "%y%m%d%H%M",300,0,168,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rg_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDG {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"8\">\n"
                               "    <north tailx=\"545\" taily=\"350\" headx=\"555\" heady=\"68\" dist=\"54\" />\n"
                               "    <point lat=\"N52 08.07493\" lon=\"E007 41.08014\" x=\"545\" y=\"350\">EDDG</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDH (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/rh_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rh_{0}.jpg"),
                               "%y%m%d%H%M",300,0,175,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rh_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDH {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10\">\n"
                               "    <north tailx=\"472\" taily=\"366\" headx=\"471\" heady=\"87\" dist=\"54\" />\n"
                               "    <point lat=\"N53 37.65432\" lon=\"E009 58.95666\" x=\"472\" y=\"366\">EDDH</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDK (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/rk_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rk_{0}.jpg"),
                               "%y%m%d%H%M",300,0,192,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rk_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDK {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"7\">\n"
                               "    <north tailx=\"500\" taily=\"344\" headx=\"514\" heady=\"61\" dist=\"54\" />\n"
                               "    <point lat=\"N50 51.89381\" lon=\"E007 08.37150\" x=\"500\" y=\"344\">EDDK</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDL (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/rl_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rl_{0}.jpg"),
                               "%y%m%d%H%M",300,0,181,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rl_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDL {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"7\">\n"
                               "    <north tailx=\"496\" taily=\"359\" headx=\"512\" heady=\"76\" dist=\"54\" />\n"
                               "    <point lat=\"N51 17.26772\" lon=\"E006 46.14994\" x=\"496\" y=\"359\">EDDL</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDM (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/rm_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rm_{0}.jpg"),
                               "%y%m%d%H%M",300,0,175,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rm_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDM {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"12\">\n"
                               "    <north tailx=\"591\" taily=\"323\" headx=\"583\" heady=\"35\" dist=\"54\" />\n"
                               "    <point lat=\"N48 20.56508\" lon=\"E011 46.67237\" x=\"591\" y=\"323\">EDDM</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDN (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/rn_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rn_{0}.jpg"),
                               "%y%m%d%H%M",300,0,173,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rn_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDN {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"11\">\n"
                               "    <north tailx=\"534\" taily=\"313\" headx=\"529\" heady=\"25\" dist=\"54\" />\n"
                               "    <point lat=\"N49 29.92459\" lon=\"E011 04.65479\" x=\"534\" y=\"313\">EDDN</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDP (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/rp_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rp_{0}.jpg"),
                               "%y%m%d%H%M",300,0,175,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rp_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDP {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"12\">\n"
                               "    <north tailx=\"427\" taily=\"374\" headx=\"416\" heady=\"92\" dist=\"54\" />\n"
                               "    <point lat=\"N51 25.94681\" lon=\"E012 14.49809\" x=\"427\" y=\"374\">EDDP</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDR (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/rr_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rr_{0}.jpg"),
                               "%y%m%d%H%M",300,0,189,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rr_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDR {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"7\">\n"
                               "    <north tailx=\"476\" taily=\"368\" headx=\"490\" heady=\"82\" dist=\"54\" />\n"
                               "    <point lat=\"N49 12.87427\" lon=\"E007 06.59642\" x=\"476\" y=\"368\">EDDR</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDS (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/rs_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rs_{0}.jpg"),
                               "%y%m%d%H%M",300,0,177,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rs_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDS {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"9\">\n"
                               "    <north tailx=\"456\" taily=\"315\" headx=\"461\" heady=\"27\" dist=\"54\" />\n"
                               "    <point lat=\"N48 41.39264\" lon=\"E009 13.31794\" x=\"456\" y=\"315\">EDDS</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDT (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/rt_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rt_{0}.jpg"),
                               "%y%m%d%H%M",300,0,168,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rt_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDT {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"13\">\n"
                               "    <north tailx=\"622\" taily=\"355\" headx=\"606\" heady=\"74\" dist=\"54\" />\n"
                               "    <point lat=\"N52 33.50436\" lon=\"E013 17.55721\" x=\"622\" y=\"355\">EDDT</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDV (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/rv_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rv_{0}.jpg"),
                               "%y%m%d%H%M",300,0,170,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rv_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDV {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"10\">\n"
                               "    <north tailx=\"515\" taily=\"383\" headx=\"517\" heady=\"102\" dist=\"54\" />\n"
                               "    <point lat=\"N52 27.87076\" lon=\"E009 41.38246\" x=\"515\" y=\"383\">EDDV</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])
            self.store.append(["Radar EDDW (compressed)",
                               "http://sailer.dynip.lugs.ch/vfrnav/imgcompr.php?src=rad/rw_{0}.png",
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rw_{0}.jpg"),
                               "%y%m%d%H%M",300,0,170,
                               os.path.expanduser("~/.vfrnav/bitmapmaps/wetterdl/rad_rw_{0}.xml"),
                               "<?xml version=\"1.0\"?>\n"
                               "<MovingMap>\n"
                               "  <map name=\"Radar EDDW {2}\" file=\"{0}\" time=\"{1}\" scale=\"1000000\" projection=\"linear\" reflon=\"9\">\n"
                               "    <north tailx=\"543\" taily=\"363\" headx=\"548\" heady=\"83\" dist=\"54\" />\n"
                               "    <point lat=\"N53 02.67258\" lon=\"E008 47.29919\" x=\"543\" y=\"363\">EDDW</point>\n"
                               "  </map>\n"
                               "</MovingMap>"])

        scrolled_window = Gtk.ScrolledWindow()
        scrolled_window.set_border_width(10)
        scrolled_window.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        vbox.pack_start(scrolled_window, True, True, 0)

        self.tree = Gtk.TreeView(self.store)
        scrolled_window.add_with_viewport(self.tree)
        renderer = Gtk.CellRendererText()
        column = Gtk.TreeViewColumn("Chart", renderer, text=0)
        self.tree.append_column(column)
        renderer = Gtk.CellRendererText()
        renderer.set_property('xalign', 1)
        column = Gtk.TreeViewColumn("Size", renderer, text=6)
        self.tree.append_column(column)
        select = self.tree.get_selection()
        select.connect("changed", self.on_tree_selection_changed)
        select.set_mode(Gtk.SelectionMode.MULTIPLE)

        hbox = Gtk.HBox(spacing=6)
        vbox.pack_start(hbox, False, True, 0)

        label = Gtk.Label("Last # charts:")
        hbox.pack_start(label, False, True, 0)

        self.nrdownload = Gtk.SpinButton()
        adjustment = Gtk.Adjustment(1, 1, 10, 1, 1, 0)
        self.nrdownload.set_adjustment(adjustment)
        self.nrdownload.set_numeric(True)
        hbox.pack_start(self.nrdownload, False, False, 0)

        self.buttondownload = Gtk.Button("Download")
        self.buttondownload.connect("clicked", self.on_download_clicked)
        self.buttondownload.set_sensitive(False)
        hbox.pack_start(self.buttondownload, True, True, 0)

        buttonquit = Gtk.Button(stock=Gtk.STOCK_QUIT)
        buttonquit.connect("clicked", self.on_quit_clicked)
        hbox.pack_start(buttonquit, True, True, 0)

    def on_download_clicked(self, button):
        print "\"Download\" button was clicked"
        selection = self.tree.get_selection()
        model, rows = selection.get_selected_rows()
        dlqueue = []
        for r in rows[:]:
            it = model.get_iter(r)
            dlqueue.append([self.store[it][1], self.store[it][2], self.store[it][3],
                            self.store[it][4], self.store[it][5], self.nrdownload.get_value(),
                            self.store[it][6], self.store[it][7], self.store[it][8]])
        self.hide()
        dlwin = DownloadWindow()
        dlwin.show_all()
        dlwin.addqueue(dlqueue)

    def on_quit_clicked(self, button):
        Gtk.main_quit()

    def on_tree_selection_changed(self, selection):
        model, rows = selection.get_selected_rows()
        self.buttondownload.set_sensitive(len(rows) > 0)
        print len(rows), " selected"
        for r in rows[:]:
            it = model.get_iter(r)
            print "Item: ", self.store[it][0]
            

win = MainWindow()
win.show_all()
Gtk.main()
