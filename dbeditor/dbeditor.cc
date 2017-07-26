/***************************************************************************
 *   Copyright (C) 2007, 2008, 2009, 2012 by Thomas Sailer                 *
 *   t.sailer@alumni.ethz.ch                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "sysdeps.h"
#include "dbeditor.h"
#include "sysdepsgui.h"

ComboCellRenderer1::ComboCellRenderer1(const std::string& pname, int pid, Glib::ValueBase& propval)
        : Glib::ObjectBase(typeid(ComboCellRenderer1)), Gtk::CellRendererCombo(),
          m_property_paramspec(0), m_property_name(pname), m_property_id(pid), m_property_value(propval)
{
}

void ComboCellRenderer1::construct(void)
{
        GObjectClass *goclass = G_OBJECT_GET_CLASS(gobj());
        GParamSpec *paramspec = g_object_class_find_property(goclass, get_property_name());
        if (paramspec) {
                g_param_spec_unref(m_property_paramspec);
                m_property_paramspec = paramspec;
                g_param_spec_ref(m_property_paramspec);
        } else {
                goclass->get_property = &ComboCellRenderer1::custom_get_property_callback;
                goclass->set_property = &ComboCellRenderer1::custom_set_property_callback;
                g_object_class_install_property(goclass, get_property_id(), m_property_paramspec);
        }
        std::cerr << "ComboCellRenderer1::ComboCellRenderer1: pspec " << G_PARAM_SPEC_VALUE_TYPE(m_property_paramspec)
                  << " prop " << G_VALUE_TYPE(m_property_value.gobj()) << std::endl;
        g_assert(G_PARAM_SPEC_VALUE_TYPE(m_property_paramspec) == G_VALUE_TYPE(m_property_value.gobj()));
}

ComboCellRenderer1::~ComboCellRenderer1(void)
{
        if (m_property_paramspec)
                g_param_spec_unref(m_property_paramspec);
}

void ComboCellRenderer1::custom_get_property_callback(GObject * object, unsigned int property_id, GValue * value, GParamSpec * param_spec)
{
        Glib::ObjectBase *const wrapper = Glib::ObjectBase::_get_current_wrapper(object);
        if (!wrapper)
                return;
        ComboCellRenderer1 *renderer = dynamic_cast<ComboCellRenderer1 *>(wrapper);
        if (!renderer)
                return;
        if (property_id == renderer->m_property_id) {
                if (renderer->m_property_paramspec == param_spec) {
                        g_value_copy(renderer->m_property_value.gobj(), value);
                } else {
                        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, param_spec);
                }
        } else {
                G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, param_spec);
        }
}

void ComboCellRenderer1::custom_set_property_callback(GObject * object, unsigned int property_id, const GValue * value, GParamSpec * param_spec)
{
        Glib::ObjectBase *const wrapper = Glib::ObjectBase::_get_current_wrapper(object);
        if (!wrapper)
                return;
        ComboCellRenderer1 *renderer = dynamic_cast<ComboCellRenderer1 *>(wrapper);
        if (!renderer)
                return;
        //std::cerr << "ComboCellRenderer1::custom_set_property_callback: prop id: " << property_id << std::endl;
        if (property_id == renderer->m_property_id) {
                if (renderer->m_property_paramspec == param_spec) {
                        g_value_copy(value, renderer->m_property_value.gobj());
                        g_object_notify(object, g_param_spec_get_name(param_spec));
                } else {
                        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, param_spec);
                }
        } else {
                G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, param_spec);
        }
}

template<class T> ComboCellRenderer<T>::ModelColumns::ModelColumns(void)
{
        add(m_col_value);
        add(m_col_text);
}

template<class T> ComboCellRenderer<T>::ComboCellRenderer(const std::string & pname, int pid)
        : Glib::ObjectBase(typeid(ComboCellRenderer)), ComboCellRenderer1(pname, pid, m_property_value),
          m_model_columns(), m_model(0)
{
        m_property_value.init(m_property_value.value_type());
        m_property_paramspec = m_property_value.create_param_spec(get_property_name());
        g_param_spec_ref(m_property_paramspec);
        construct();
        connect_property_changed(get_property_name(), sigc::mem_fun(*this, &ComboCellRenderer::property_changed));
        connect_property_changed("text", sigc::mem_fun(*this, &ComboCellRenderer::text_changed));
        Gtk::CellRendererCombo::signal_edited().connect(sigc::mem_fun(*this, &ComboCellRenderer::on_my_edited));
        m_model = Gtk::TreeStore::create(m_model_columns);
        set_property("model", m_model);
        set_property("text-column", 1);
        set_property("has-entry", false);
        set_property("editable", true);
}

template<class T> void ComboCellRenderer<T>::on_my_edited(const Glib::ustring & path, const Glib::ustring & new_text)
{
        //std::cerr << typeid(ComboCellRenderer<T>).name() << "::on_my_edited: new_text: " << new_text << std::endl;
        Gtk::TreeModel::iterator iter;
        for (iter = m_model->children().begin(); iter; iter++) {
                if ((*iter)[m_model_columns.m_col_text] == new_text)
                        break;
                Gtk::TreeModel::iterator citer = (*iter)->children().begin();
                for (; citer; citer++) {
                        if ((*citer)[m_model_columns.m_col_text] == new_text) {
                                break;
                        }
                }
                if (citer) {
                        iter = citer;
                        break;
                }
        }
        if (!iter)
                m_model->children().begin();
        T val(iter ? (*iter)[m_model_columns.m_col_value] : T(0));
        //std::cerr << typeid(ComboCellRenderer<T>).name() << "::on_my_edited: value: " << val << std::endl;
        set_property(get_property_name(), val);
        m_signal_edited.emit(path, new_text, val);
}

template<class T> void ComboCellRenderer<T>::text_changed(void)
{
        Glib::ustring text;
        get_property("text", text);
        //std::cerr << typeid(ComboCellRenderer<T>).name() << "::text_changed: text: " << text << std::endl;
        Gtk::TreeModel::iterator iter;
        for (iter = m_model->children().begin(); iter; iter++) {
                if ((*iter)[m_model_columns.m_col_text] == text)
                        break;
                Gtk::TreeModel::iterator citer = (*iter)->children().begin();
                for (; citer; citer++) {
                        if ((*citer)[m_model_columns.m_col_text] == text) {
                                break;
                        }
                }
                if (citer) {
                        iter = citer;
                        break;
                }
        }
        if (!iter)
                m_model->children().begin();
        if (iter) {
                T val((*iter)[m_model_columns.m_col_value]);
                T val1;
                get_property(get_property_name(), val1);
                if (val1 != val)
                        set_property(get_property_name(), val);
        }
}

template<class T> void ComboCellRenderer<T>::property_changed(void)
{
        T val;
        get_property(get_property_name(), val);
        //std::cerr << typeid(ComboCellRenderer<T>).name() << "::property_changed: val " << val << std::endl;
        Gtk::TreeModel::iterator iter;
        for (iter = m_model->children().begin(); iter; iter++) {
                if ((*iter)[m_model_columns.m_col_value] == val)
                        break;
                Gtk::TreeModel::iterator citer = (*iter)->children().begin();
                for (; citer; citer++) {
                        if ((*citer)[m_model_columns.m_col_value] == val) {
                                break;
                        }
                }
                if (citer) {
                        iter = citer;
                        break;
                }
        }
        if (!iter)
                m_model->children().begin();
        if (iter) {
                Glib::ustring text((*iter)[m_model_columns.m_col_text]);
                //std::cerr << "ComboCellRenderer<T>::property_changed: val " << val << " text " << text << std::endl;
                Glib::ustring text1;
                get_property("text", text1);
                if (text1 != text)
                        set_property("text", text);
        }
}

template<class T> void ComboCellRenderer<T>::add_value(T val, const Glib::ustring & text)
{
        Gtk::TreeModel::iterator iter(m_model->append());
        (*iter)[m_model_columns.m_col_value] = val;
        (*iter)[m_model_columns.m_col_text] = text;
}

template<class T> T ComboCellRenderer<T>::get_value(void)
{
        T val;
        get_property(get_property_name(), val);
        return val;
}

template<class T> void ComboCellRenderer<T>::set_value(T val)
{
        set_property(get_property_name(), val);
}

template<class T> NumericCellRenderer<T>::NumericCellRenderer(const std::string& pname, int pid)
        : Glib::ObjectBase(typeid(NumericCellRenderer<T>)), Gtk::CellRendererText(),
          m_property_name(pname), m_property_id(pid),
          m_property_paramspec(0), m_property_value()
{
        m_property_value.init(m_property_value.value_type());
        GObjectClass *goclass = G_OBJECT_GET_CLASS(gobj());
        m_property_paramspec = g_object_class_find_property(goclass, get_property_name());
        if (!m_property_paramspec) {
                goclass->get_property = &NumericCellRenderer<T>::custom_get_property_callback;
                goclass->set_property = &NumericCellRenderer<T>::custom_set_property_callback;
                m_property_paramspec = m_property_value.create_param_spec(get_property_name());
                g_object_class_install_property(goclass, get_property_id(), m_property_paramspec);
        }
        g_param_spec_ref(m_property_paramspec);
        g_assert(G_PARAM_SPEC_VALUE_TYPE(m_property_paramspec) == G_VALUE_TYPE(m_property_value.gobj()));
        connect_property_changed(get_property_name(), sigc::mem_fun(*this, &NumericCellRenderer<T>::property_changed));
        connect_property_changed("text", sigc::mem_fun(*this, &NumericCellRenderer<T>::text_changed));
        Gtk::CellRendererText::signal_edited().connect(sigc::mem_fun(*this, &NumericCellRenderer<T>::on_my_edited));
}

template<class T> NumericCellRenderer<T>::~NumericCellRenderer(void)
{
        if (m_property_paramspec)
                g_param_spec_unref(m_property_paramspec);
}

template<class T> void NumericCellRenderer<T>::custom_get_property_callback(GObject * object, unsigned int property_id, GValue * value, GParamSpec * param_spec)
{
        Glib::ObjectBase *const wrapper = Glib::ObjectBase::_get_current_wrapper(object);
        if (!wrapper)
                return;
        NumericCellRenderer<T> *renderer = dynamic_cast<NumericCellRenderer<T> *>(wrapper);
        if (!renderer)
                return;
        if (property_id == renderer->m_property_id) {
                if (renderer->m_property_paramspec == param_spec) {
                        g_value_copy(renderer->m_property_value.gobj(), value);
                } else {
                        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, param_spec);
                }
        } else {
                G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, param_spec);
        }
}

template<class T> void NumericCellRenderer<T>::custom_set_property_callback(GObject * object, unsigned int property_id, const GValue * value, GParamSpec * param_spec)
{
        Glib::ObjectBase *const wrapper = Glib::ObjectBase::_get_current_wrapper(object);
        if (!wrapper)
                return;
        NumericCellRenderer<T> *renderer = dynamic_cast<NumericCellRenderer<T> *>(wrapper);
        if (!renderer)
                return;
        if (property_id == renderer->m_property_id) {
                if (renderer->m_property_paramspec == param_spec) {
                        g_value_copy(value, renderer->m_property_value.gobj());
                        g_object_notify(object, g_param_spec_get_name(param_spec));
                } else {
                        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, param_spec);
                }
        } else {
                G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, param_spec);
        }
}

template<class T> void NumericCellRenderer<T>::on_my_edited(const Glib::ustring & path, const Glib::ustring & new_text)
{
        //std::cerr << "NumericCellRenderer<T>::on_my_edited: " << new_text << std::endl;
        T val(to_numeric(new_text));
        set_property(get_property_name(), val);
        m_signal_edited(path, new_text, val);
}

template<class T> void NumericCellRenderer<T>::text_changed(void)
{
        Glib::ustring text;
        get_property("text", text);
        //std::cerr << "NumericCellRenderer<T>::text_changed: " << text << std::endl;
        T val(to_numeric(text));
        T val1;
        get_property(get_property_name(), val1);
        if (val1 != val)
                set_property(get_property_name(), val);
}

template<class T> void NumericCellRenderer<T>::property_changed(void)
{
        T val;
        get_property(get_property_name(), val);
        //std::cerr << "NumericCellRenderer<T>::property_changed: " << val << std::endl;
        Glib::ustring text(to_text(val));
        Glib::ustring text1;
        get_property("text", text1);
        if (text != text1)
                set_property("text", text);
}

template<class T> T NumericCellRenderer<T>::get_value(void)
{
        T val;
        get_property(get_property_name(), val);
        return val;
}

template<class T> void NumericCellRenderer<T>::set_value(T val)
{
        set_property(get_property_name(), val);
}

LabelPlacementRenderer::LabelPlacementRenderer(void)
        : Glib::ObjectBase(0), // Mark this class as gtkmmproc-generated, rather than a custom class, to allow vfunc optimisations.
          // TODO: This should not be necessary - our gtkmm callbacks are somehow preventing the popup from appearing.
          ComboCellRenderer<NavaidsDb::Navaid::label_placement_t>("vfrnav-labelplacement", 0x100)
{
        add_value(NavaidsDb::Navaid::label_off);
        add_value(NavaidsDb::Navaid::label_center);
        add_value(NavaidsDb::Navaid::label_n);
        add_value(NavaidsDb::Navaid::label_e);
        add_value(NavaidsDb::Navaid::label_s);
        add_value(NavaidsDb::Navaid::label_w);
        add_value(NavaidsDb::Navaid::label_any);
}

NavaidTypeRenderer::NavaidTypeRenderer(void)
        : Glib::ObjectBase(0), // Mark this class as gtkmmproc-generated, rather than a custom class, to allow vfunc optimisations.
          // TODO: This should not be necessary - our gtkmm callbacks are somehow preventing the popup from appearing.
          ComboCellRenderer<NavaidsDb::Navaid::navaid_type_t>("vfrnav-navaidtype", 0x101)
{
        add_value(NavaidsDb::Navaid::navaid_invalid);
        add_value(NavaidsDb::Navaid::navaid_vor);
        add_value(NavaidsDb::Navaid::navaid_vordme);
        add_value(NavaidsDb::Navaid::navaid_dme);
        add_value(NavaidsDb::Navaid::navaid_ndb);
}

WaypointUsageRenderer::WaypointUsageRenderer(void)
        : Glib::ObjectBase(0), // Mark this class as gtkmmproc-generated, rather than a custom class, to allow vfunc optimisations.
          // TODO: This should not be necessary - our gtkmm callbacks are somehow preventing the popup from appearing.
          ComboCellRenderer<char>("vfrnav-waypointusage", 0x102)
{
        add_value(WaypointsDb::Waypoint::usage_invalid);
        add_value(WaypointsDb::Waypoint::usage_highlevel);
        add_value(WaypointsDb::Waypoint::usage_lowlevel);
        add_value(WaypointsDb::Waypoint::usage_bothlevel);
        add_value(WaypointsDb::Waypoint::usage_rnav);
        add_value(WaypointsDb::Waypoint::usage_terminal);
        add_value(WaypointsDb::Waypoint::usage_vfr);
        add_value(WaypointsDb::Waypoint::usage_user);
}

WaypointTypeRenderer::WaypointTypeRenderer(void)
        : Glib::ObjectBase(0), // Mark this class as gtkmmproc-generated, rather than a custom class, to allow vfunc optimisations.
          // TODO: This should not be necessary - our gtkmm callbacks are somehow preventing the popup from appearing.
          ComboCellRenderer<char>("vfrnav-waypointtype", 0x103)
{
        add_value(WaypointsDb::Waypoint::type_invalid);
        add_value(WaypointsDb::Waypoint::type_unknown);
        add_value(WaypointsDb::Waypoint::type_adhp);
        add_value(WaypointsDb::Waypoint::type_icao);
        add_value(WaypointsDb::Waypoint::type_coord);
        add_value(WaypointsDb::Waypoint::type_other);
}

AirwayTypeRenderer::AirwayTypeRenderer(void)
        : Glib::ObjectBase(0), // Mark this class as gtkmmproc-generated, rather than a custom class, to allow vfunc optimisations.
          // TODO: This should not be necessary - our gtkmm callbacks are somehow preventing the popup from appearing.
          ComboCellRenderer<AirwaysDb::Airway::airway_type_t>("vfrnav-airwaytype", 0x104)
{
        add_value(AirwaysDb::Airway::airway_invalid);
        add_value(AirwaysDb::Airway::airway_low);
        add_value(AirwaysDb::Airway::airway_high);
        add_value(AirwaysDb::Airway::airway_both);
        add_value(AirwaysDb::Airway::airway_user);
}

AirspaceClassRenderer::AirspaceClassRenderer(void)
        : Glib::ObjectBase(0), // Mark this class as gtkmmproc-generated, rather than a custom class, to allow vfunc optimisations.
          // TODO: This should not be necessary - our gtkmm callbacks are somehow preventing the popup from appearing.
          ComboCellRenderer<AirspaceClassType>("vfrnav-airspaceclasstype", 0x105)
{
        for (char cls = 'A'; cls <= 'G'; cls++) {
                Gtk::TreeStore::Row row(*(m_model->append()));
                row[m_model_columns.m_col_value] = AirspaceClassType(cls, 0);
                row[m_model_columns.m_col_text] = AirspacesDb::Airspace::get_class_string(cls, 'A');
                for (char tc = 'A'; tc <= 'D'; tc++) {
                        Gtk::TreeStore::Row crow(*(m_model->append(row.children())));
                        crow[m_model_columns.m_col_value] = AirspaceClassType(cls, tc);
                        crow[m_model_columns.m_col_text] = get_class_string(cls, tc);
                }
        }
        static const char suaschars[] = "ADMPRTW";
        for (const char *cp = suaschars; *cp; cp++) {
                add_value(AirspaceClassType(*cp, AirspacesDb::Airspace::typecode_specialuse),
			  AirspacesDb::Airspace::get_class_string(*cp, AirspacesDb::Airspace::typecode_specialuse));
        }
	// EAD codes
	{
                Gtk::TreeStore::Row row(*(m_model->append()));
		row[m_model_columns.m_col_value] = AirspaceClassType(AirspacesDb::Airspace::bdryclass_ead_ead, AirspacesDb::Airspace::typecode_ead);
                row[m_model_columns.m_col_text] = get_class_string(AirspacesDb::Airspace::bdryclass_ead_ead, AirspacesDb::Airspace::typecode_ead);
		static const char codetypes[] = {
			AirspacesDb::Airspace::bdryclass_ead_a,
			AirspacesDb::Airspace::bdryclass_ead_adiz,
			AirspacesDb::Airspace::bdryclass_ead_ama,
			AirspacesDb::Airspace::bdryclass_ead_arinc,
			AirspacesDb::Airspace::bdryclass_ead_asr,
			AirspacesDb::Airspace::bdryclass_ead_awy,
			AirspacesDb::Airspace::bdryclass_ead_cba,
			AirspacesDb::Airspace::bdryclass_ead_class,
			AirspacesDb::Airspace::bdryclass_ead_cta,
			AirspacesDb::Airspace::bdryclass_ead_cta_p,
			AirspacesDb::Airspace::bdryclass_ead_ctr,
			AirspacesDb::Airspace::bdryclass_ead_ctr_p,
			AirspacesDb::Airspace::bdryclass_ead_d_amc,
			AirspacesDb::Airspace::bdryclass_ead_d,
			AirspacesDb::Airspace::bdryclass_ead_d_other,
			AirspacesDb::Airspace::bdryclass_ead_ead,
			AirspacesDb::Airspace::bdryclass_ead_fir,
			AirspacesDb::Airspace::bdryclass_ead_fir_p,
			AirspacesDb::Airspace::bdryclass_ead_nas,
			AirspacesDb::Airspace::bdryclass_ead_no_fir,
			AirspacesDb::Airspace::bdryclass_ead_oca,
			AirspacesDb::Airspace::bdryclass_ead_oca_p,
			AirspacesDb::Airspace::bdryclass_ead_ota,
			AirspacesDb::Airspace::bdryclass_ead_part,
			AirspacesDb::Airspace::bdryclass_ead_p,
			AirspacesDb::Airspace::bdryclass_ead_political,
			AirspacesDb::Airspace::bdryclass_ead_protect,
			AirspacesDb::Airspace::bdryclass_ead_r_amc,
			AirspacesDb::Airspace::bdryclass_ead_ras,
			AirspacesDb::Airspace::bdryclass_ead_rca,
			AirspacesDb::Airspace::bdryclass_ead_r,
			AirspacesDb::Airspace::bdryclass_ead_sector_c,
			AirspacesDb::Airspace::bdryclass_ead_sector,
			AirspacesDb::Airspace::bdryclass_ead_tma,
			AirspacesDb::Airspace::bdryclass_ead_tma_p,
			AirspacesDb::Airspace::bdryclass_ead_tra,
			AirspacesDb::Airspace::bdryclass_ead_tsa,
			AirspacesDb::Airspace::bdryclass_ead_uir,
			AirspacesDb::Airspace::bdryclass_ead_uir_p,
			AirspacesDb::Airspace::bdryclass_ead_uta,
			AirspacesDb::Airspace::bdryclass_ead_w
		};
		for (unsigned int i = 0; i < sizeof(codetypes)/sizeof(codetypes[0]); ++i) {
			Gtk::TreeStore::Row crow(*(m_model->append(row.children())));
                        crow[m_model_columns.m_col_value] = AirspaceClassType(codetypes[i], AirspacesDb::Airspace::typecode_ead);
                        crow[m_model_columns.m_col_text] = get_class_string(codetypes[i], AirspacesDb::Airspace::typecode_ead);
		}
	}
}

Glib::ustring AirspaceClassRenderer::get_class_string(char bc, uint8_t tc)
{
        Glib::ustring s(AirspacesDb::Airspace::get_class_string(bc, tc));
        if (tc == AirspacesDb::Airspace::typecode_specialuse || tc == AirspacesDb::Airspace::typecode_ead)
                return s;
        s += " (" + AirspacesDb::Airspace::get_type_string(tc) + ")";
        return s;
}

AirspaceShapeRenderer::AirspaceShapeRenderer(void)
        : Glib::ObjectBase(0), // Mark this class as gtkmmproc-generated, rather than a custom class, to allow vfunc optimisations.
          // TODO: This should not be necessary - our gtkmm callbacks are somehow preventing the popup from appearing.
          ComboCellRenderer<char>("vfrnav-airspaceshape", 0x106)
{
        static const char shapes[] = "-BHLRCGAEF";
        for (const char *cp = shapes; *cp; cp++)
                add_value(*cp);
}

const Glib::ustring& AirspaceShapeRenderer::get_shape_string(char shp)
{
        switch (shp) {
	case '-':
	{
		static const Glib::ustring r("Line Segment");
		return r;
	}

	case 'L':
	{
		static const Glib::ustring r("Counterclockwise Arc");
		return r;
	}

	case 'R':
	{
		static const Glib::ustring r("Clockwise Arc");
		return r;
	}

	case 'C':
	{
		static const Glib::ustring r("Circle");
		return r;
	}

	case 'B':
	{
		static const Glib::ustring r("Great Circle (Line)");
		return r;
	}

	case 'H':
	{
		static const Glib::ustring r("Rhumb Line");
		return r;
	}

	case 'G':
	{
		static const Glib::ustring r("Generalized");
		return r;
	}

	case 'A':
	{
		static const Glib::ustring r("Point");
		return r;
	}

	case 'E':
	{
		static const Glib::ustring r("Endpoint");
		return r;
	}

	case 'F':
	{
		static const Glib::ustring r("Border (Frontier)");
		return r;
	}

	default:
	{
		std::cerr << "invalid shape: " << shp << std::endl;
		static const Glib::ustring r("Invalid");
		return r;
	}
        }
}

AirspaceOperatorRenderer::AirspaceOperatorRenderer(void)
        : Glib::ObjectBase(0), // Mark this class as gtkmmproc-generated, rather than a custom class, to allow vfunc optimisations.
          // TODO: This should not be necessary - our gtkmm callbacks are somehow preventing the popup from appearing.
          ComboCellRenderer<AirspacesDb::Airspace::Component::operator_t>("vfrnav-airspaceoperator", 0x107)
{
	for (AirspacesDb::Airspace::Component::operator_t opr = AirspacesDb::Airspace::Component::operator_set;
	     opr <= AirspacesDb::Airspace::Component::operator_intersect;
	     opr = (AirspacesDb::Airspace::Component::operator_t)(opr + 1))
		add_value(opr);
}

AirportTypeRenderer::AirportTypeRenderer(void)
        : Glib::ObjectBase(0), // Mark this class as gtkmmproc-generated, rather than a custom class, to allow vfunc optimisations.
          // TODO: This should not be necessary - our gtkmm callbacks are somehow preventing the popup from appearing.
          ComboCellRenderer<char>("vfrnav-airporttype", 0x108)
{
        for (char c = 'A'; c <= 'D'; c++)
                add_value(c);
}

AirportVFRAltCodeRenderer::AirportVFRAltCodeRenderer(void)
        : Glib::ObjectBase(0), // Mark this class as gtkmmproc-generated, rather than a custom class, to allow vfunc optimisations.
          // TODO: This should not be necessary - our gtkmm callbacks are somehow preventing the popup from appearing.
          ComboCellRenderer<AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_t>("vfrnav-airportvfraltcode", 0x109)
{
        add_value(AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_atorabove, ">=");
        add_value(AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_atorbelow, "<=");
        add_value(AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_assigned, "assigned");
        add_value(AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_altspecified, "specified");
        add_value(AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_recommendedalt, "recommended");
}

AirportVFRPathCodeRenderer::AirportVFRPathCodeRenderer(void)
        : Glib::ObjectBase(0), // Mark this class as gtkmmproc-generated, rather than a custom class, to allow vfunc optimisations.
          // TODO: This should not be necessary - our gtkmm callbacks are somehow preventing the popup from appearing.
          ComboCellRenderer<AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_t>("vfrnav-airportvfrpathcode", 0x10A)
{
        add_value(AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star);
        add_value(AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid);
        add_value(AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_arrival);
        add_value(AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_departure);
        add_value(AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_holding);
        add_value(AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_trafficpattern);
        add_value(AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_vfrtransition);
        add_value(AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_other);
}

AirportVFRPointSymbolRenderer::AirportVFRPointSymbolRenderer(void)
        : Glib::ObjectBase(0), // Mark this class as gtkmmproc-generated, rather than a custom class, to allow vfunc optimisations.
          // TODO: This should not be necessary - our gtkmm callbacks are somehow preventing the popup from appearing.
          ComboCellRenderer<char>("vfrnav-airportvfrptsym", 0x10B)
{
        add_value(' ', "No Symbol");
        add_value('C', "Compulsory");
        add_value('N', "Noncompulsory");
}

AirportRunwayLightRenderer::AirportRunwayLightRenderer(void)
        : Glib::ObjectBase(0), // Mark this class as gtkmmproc-generated, rather than a custom class, to allow vfunc optimisations.
          // TODO: This should not be necessary - our gtkmm callbacks are somehow preventing the popup from appearing.
          ComboCellRenderer<DbBaseElements::Airport::Runway::light_t>("vfrnav-airportrwylight", 0x10C)
{
        for (DbBaseElements::Airport::Runway::light_t x(DbBaseElements::Airport::Runway::light_off); x < 64; ++x) {
                Glib::ustring s(DbBaseElements::Airport::Runway::get_light_string(x));
                if (s.empty())
                        continue;
                add_value(x, s);
        }
}

DateTimeCellRenderer::DateTimeCellRenderer(void)
        : Glib::ObjectBase(typeid(DateTimeCellRenderer)),
          NumericCellRenderer<time_t>("vfrnav-datetime", 0x200)
{
}

time_t DateTimeCellRenderer::to_numeric(const Glib::ustring & text)
{
        return Conversions::time_parse(text);
}

Glib::ustring DateTimeCellRenderer::to_text(time_t val)
{
        Glib::ustring t(Conversions::time_str("%Y-%m-%d %H:%M:%S", val));
        //std::cerr << "DateTimeCellRenderer::to_text: text " << t << " val " << val << std::endl;
        return t;
}

DateTimeFrac8CellRenderer::DateTimeFrac8CellRenderer(void)
        : Glib::ObjectBase(typeid(DateTimeFrac8CellRenderer)),
                           NumericCellRenderer<uint64_t>("vfrnav-datetimefrac8", 0x203)
{
}

uint64_t DateTimeFrac8CellRenderer::to_numeric(const Glib::ustring & text)
{
        return Conversions::timefrac8_parse(text);
}

Glib::ustring DateTimeFrac8CellRenderer::to_text(uint64_t val)
{
        Glib::ustring t(Conversions::timefrac8_str("%Y-%m-%d %H:%M:%S.%f", val));
        //std::cerr << "DateTimeFrac8CellRenderer::to_text: text " << t << " val " << val << std::endl;
        return t;
}

CoordCellRenderer::CoordCellRenderer()
        : Glib::ObjectBase(typeid(CoordCellRenderer)),
          NumericCellRenderer<Point>("vfrnav-coordinate", 0x201)
{
}

Point CoordCellRenderer::to_numeric(const Glib::ustring& text)
{
        Point pt;
	if (text.empty())
		pt.set_invalid();
	else
		pt.set_str(text);
        return pt;
}

Glib::ustring CoordCellRenderer::to_text(Point val)
{
	if (val.is_invalid())
		return Glib::ustring();
	return val.get_lat_str2() + " " + val.get_lon_str2();
}

LatLonCellRenderer::LatLonCellRenderer(bool latitude_mode)
        : Glib::ObjectBase(typeid(LatLonCellRenderer)),
          NumericCellRenderer<Point::coord_t>("vfrnav-latlon", 0x205), m_latitude_mode(latitude_mode)
{
}

Point::coord_t LatLonCellRenderer::to_numeric(const Glib::ustring& text)
{
        Point pt;
	pt.set_str(text);
	return m_latitude_mode ? pt.get_lat() : pt.get_lon();
}

Glib::ustring LatLonCellRenderer::to_text(Point::coord_t val)
{
	Glib::ustring t(m_latitude_mode ? Point(0, val).get_lat_str2() : Point(val, 0).get_lon_str2());
        return t;
}

FrequencyCellRenderer::FrequencyCellRenderer(void)
        : Glib::ObjectBase(typeid(FrequencyCellRenderer)),
          NumericCellRenderer<uint32_t>("vfrnav-frequency", 0x202)
{
}

uint32_t FrequencyCellRenderer::to_numeric(const Glib::ustring & text)
{
        if (text.empty())
                return 0;
        uint32_t f(0);
        char *cp;
        long long int f1;
        f1 = strtoll(text.c_str(), &cp, 10);
        if (*cp == ' ' || *cp == 0) {
                f = f1 * 1000;
                goto ok;
        }
        f1 *= 1000000;
        if (*cp == '.') {
                char *cp2 = cp + 1;
                unsigned long long int f2 = strtoull(cp2, &cp, 10);
                int i = cp - cp2;
                while (i > 6) {
                        f2 /= 10;
                        i--;
                }
                while (i < 6) {
                        f2 *= 10;
                        i++;
                }
                f1 += f2;
        }
        if (*cp == 'k' || *cp == 'K')
                f1 = (f1 + 500) / 1000;
        else if (*cp == 'h' || *cp == 'H')
                f1 = (f1 + 500000) / 1000000;
        f = f1;
ok:
        //std::cerr << "to_numeric: " << text << " value " << f << std::endl;
        return f;
}

Glib::ustring FrequencyCellRenderer::to_text(uint32_t val)
{
        Glib::ustring t(Conversions::freq_str(val));
        return t;
}

AngleCellRenderer::AngleCellRenderer(void)
        : Glib::ObjectBase(typeid(AngleCellRenderer)),
          NumericCellRenderer<uint16_t>("vfrnav-angle", 0x204)
{
}

uint16_t AngleCellRenderer::to_numeric(const Glib::ustring & text)
{
        return Point::round<int,double>(strtod(text.c_str(), 0) * FPlanLeg::from_deg);
}

Glib::ustring AngleCellRenderer::to_text(uint16_t val)
{
        char buf[16];
        snprintf(buf, sizeof(buf), "%5.1f", val * FPlanLeg::to_deg);
        return buf;
}

DbSelectDialog::RegionModelColumns::RegionModelColumns(void)
{
        add(m_text);
        add(m_rect);
}

DbSelectDialog::CompareModelColumns::CompareModelColumns(void)
{
        add(m_text);
        add(m_comp);
}

DbSelectDialog::WorldMap::WorldMap(BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml)
        : Gtk::DrawingArea(castitem), m_offset(0, 0), m_rect_valid(false), m_ptrrect_valid(false), m_ptrinside(false)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &WorldMap::on_expose_event));
        signal_size_request().connect(sigc::mem_fun(*this, &WorldMap::on_size_request));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &WorldMap::on_draw));
#endif
        signal_show().connect(sigc::mem_fun(*this, &WorldMap::on_show));
        signal_hide().connect(sigc::mem_fun(*this, &WorldMap::on_hide));
        signal_button_press_event().connect(sigc::mem_fun(*this, &WorldMap::on_button_press_event));
        signal_button_release_event().connect(sigc::mem_fun(*this, &WorldMap::on_button_release_event));
        signal_motion_notify_event().connect(sigc::mem_fun(*this, &WorldMap::on_motion_notify_event));
        signal_leave_notify_event().connect(sigc::mem_fun(*this, &WorldMap::on_leave_notify_event));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

DbSelectDialog::WorldMap::~WorldMap()
{
}

void DbSelectDialog::WorldMap::set_rect(const Rect & r)
{
        if (!m_rect_valid || m_rect != r) {
                m_rect_valid = true;
                m_rect = r;
                redraw();
        }
}

void DbSelectDialog::WorldMap::invalidate_rect(void)
{
        if (m_rect_valid) {
                m_rect_valid = false;
                redraw();
        }
}

#ifdef __MINGW32__

const char *DbSelectDialog::WorldMap::pixmap_filename = "BlankMap-World_gray.svg";

void DbSelectDialog::WorldMap::load_pixmap(Glib::RefPtr<Gdk::Pixbuf>& pixbuf, int width, int height)
{
        typedef std::vector<Glib::ustring> dirs_t;
        dirs_t dirs;
        dirs.push_back(PACKAGE_DATA_DIR);
        {
                gchar *instdir(g_win32_get_package_installation_directory_of_module(0));
                if (instdir) {
                        dirs.push_back(Glib::build_filename(instdir, "share\\vfrnav"));
                        dirs.push_back(Glib::build_filename(instdir, "share"));
                        dirs.push_back(Glib::build_filename(instdir, "vfrnav"));
                        dirs.push_back(instdir);
                        dirs.push_back(Glib::build_filename(instdir, "..\\share\\vfrnav"));
                        dirs.push_back(Glib::build_filename(instdir, "..\\share"));
                        dirs.push_back(Glib::build_filename(instdir, "..\\vfrnav"));
                        g_free(instdir);
                }
        }
        for (dirs_t::const_iterator di(dirs.begin()), de(dirs.end()); di != de; ++di) {
                Glib::ustring fn(Glib::build_filename(*di, pixmap_filename));
                if (true)
                        std::cerr << "load_pixmap: trying " << fn << std::endl;
                if (!Glib::file_test(fn, Glib::FILE_TEST_IS_REGULAR))
                        continue;
#ifdef GLIBMM_EXCEPTIONS_ENABLED
		pixbuf = Gdk::Pixbuf::create_from_file(fn, width, height, true);
#else
		{
			std::auto_ptr<Glib::Error> ex;
			pixbuf = Gdk::Pixbuf::create_from_file(fn, width, height, true, ex);
			if (ex.get())
				throw *ex;
		}
#endif
                return;
        }
        throw std::runtime_error(std::string("pixmap file ") + pixmap_filename + " not found");
}

#else

const char *DbSelectDialog::WorldMap::pixmap_filename = PACKAGE_DATA_DIR "/BlankMap-World_gray.svg";

void DbSelectDialog::WorldMap::load_pixmap(Glib::RefPtr<Gdk::Pixbuf>& pixbuf, int width, int height)
{
#ifdef GLIBMM_EXCEPTIONS_ENABLED
        pixbuf = Gdk::Pixbuf::create_from_file(pixmap_filename, width, height, true);
#else
        {
                std::auto_ptr<Glib::Error> ex;
                pixbuf = Gdk::Pixbuf::create_from_file(pixmap_filename, width, height, true, ex);
                if (ex.get())
                        throw *ex;
        }
#endif
}

#endif

bool DbSelectDialog::WorldMap::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
        if (!m_pixbuf
            || (m_pixbuf->get_width() > get_width()) || (m_pixbuf->get_height() > get_height())
            || (m_pixbuf->get_width() < get_width() && m_pixbuf->get_height() < get_height())) {
                load_pixmap(get_width(), get_height());
        }
        if (m_pixbuf) {
                m_offset = Gdk::Point((get_width() - m_pixbuf->get_width()) / 2, (get_height() - m_pixbuf->get_height()) / 2);
        } else {
                m_offset = Gdk::Point(0, 0);
        }
	cr->save();
	cr->set_source_rgb(1.0, 1.0, 1.0);
	cr->paint();
	cr->restore();
        if (!m_pixbuf)
                return true;
	cr->save();
	{
		Cairo::RefPtr<Cairo::Context> cr1(Cairo::RefPtr<Cairo::Context>::cast_const(cr));
		Gdk::Cairo::set_source_pixbuf(cr1, m_pixbuf, m_offset.get_x(), m_offset.get_y());
	}
	cr->paint();
	cr->restore();
	if (!m_rect_valid && !m_ptrrect_valid && !m_ptrinside)
                return true;
	cr->save();
        if (m_rect_valid) {
                Gdk::Point p1(coord_to_screen(m_rect.get_southwest()));
                Gdk::Point p2(coord_to_screen(m_rect.get_northeast()));
                if (p2.get_x() < p1.get_x()) {
                        cr->rectangle(p1.get_x(), std::min(p1.get_y(), p2.get_y()), m_pixbuf->get_width() - p1.get_x(), abs(p2.get_y() - p1.get_y()));
                        cr->rectangle(0, std::min(p1.get_y(), p2.get_y()), p2.get_x(), abs(p2.get_y() - p1.get_y()));
                } else {
                        cr->rectangle(p1.get_x(), std::min(p1.get_y(), p2.get_y()), p2.get_x() - p1.get_x(), abs(p2.get_y() - p1.get_y()));
                }
                cr->set_source_rgb(0.0, 0.0, 0.8);
                cr->set_line_width(1.0);
                cr->stroke_preserve ();
                cr->set_source_rgba(0.0, 0.0, 0.8, 0.3);
                cr->fill();
        }
        if (m_ptrrect_valid) {
                Gdk::Point p1(coord_to_screen(m_ptrrect.get_southwest()));
                Gdk::Point p2(coord_to_screen(m_ptrrect.get_northeast()));
                if (p2.get_x() < p1.get_x()) {
                        cr->rectangle(p1.get_x(), std::min(p1.get_y(), p2.get_y()), m_pixbuf->get_width() - p1.get_x(), abs(p2.get_y() - p1.get_y()));
                        cr->rectangle(0, std::min(p1.get_y(), p2.get_y()), p2.get_x(), abs(p2.get_y() - p1.get_y()));
                } else {
                        cr->rectangle(p1.get_x(), std::min(p1.get_y(), p2.get_y()), p2.get_x() - p1.get_x(), abs(p2.get_y() - p1.get_y()));
                }
                cr->set_source_rgb(0.8, 0.0, 0.0);
                cr->set_line_width(1.0);
                cr->stroke_preserve ();
                cr->set_source_rgba(0.8, 0.0, 0.0, 0.3);
                cr->fill();
        }
        if (m_ptrinside) {
                cr->set_source_rgb(0.0, 0.0, 0.0);
                cr->set_font_size(12);
                Cairo::TextExtents ext;
                cr->get_text_extents("W", ext);
                cr->move_to(0, get_height() - 2 * ext.height - 4);
                cr->show_text(m_ptr.get_lat_str());
                cr->move_to(0, get_height() - ext.height - 2);
                cr->show_text(m_ptr.get_lon_str());
        }
	cr->restore();
	return true;
}

#ifdef HAVE_GTKMM2
bool DbSelectDialog::WorldMap::on_expose_event(GdkEventExpose *event)
{
        Glib::RefPtr<Gdk::Window> wnd(get_window());
        if (!wnd)
                return false;
        Cairo::RefPtr<Cairo::Context> cr(wnd->create_cairo_context());
	if (event) {
		GdkRectangle *rects;
		int n_rects;
		gdk_region_get_rectangles(event->region, &rects, &n_rects);
		for (int i = 0; i < n_rects; i++)
			cr->rectangle(rects[i].x, rects[i].y, rects[i].width, rects[i].height);
		cr->clip();
		g_free(rects);
	}
        return on_draw(cr);
}

void DbSelectDialog::WorldMap::on_size_request(Gtk::Requisition * requisition)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_size_request(requisition);
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        load_pixmap(200);
        if (m_pixbuf && requisition) {
                requisition->width = m_pixbuf->get_width();
                requisition->height = m_pixbuf->get_height();
        }
}
#endif

#ifdef HAVE_GTKMM3
Gtk::SizeRequestMode DbSelectDialog::WorldMap::get_request_mode_vfunc() const
{
	return Gtk::DrawingArea::get_request_mode_vfunc();
}

void DbSelectDialog::WorldMap::get_preferred_width_vfunc(int& minimum_width, int& natural_width) const
{
	minimum_width = 200;
	natural_width = 400;
}

void DbSelectDialog::WorldMap::get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const
{
	minimum_height = natural_height = 2 * width;
	Glib::RefPtr<Gdk::Pixbuf> pixbuf;
        load_pixmap(pixbuf, width, -1);
        if (pixbuf)
                minimum_height = natural_height = pixbuf->get_height();
}

void DbSelectDialog::WorldMap::get_preferred_height_vfunc(int& minimum_height, int& natural_height) const
{
	minimum_height = 400;
	natural_height = 800;
	Glib::RefPtr<Gdk::Pixbuf> pixbuf;
	load_pixmap(pixbuf, 200);
        if (pixbuf) {
		minimum_height = pixbuf->get_height();
		natural_height = pixbuf->get_height() * 2;
        }
}

void DbSelectDialog::WorldMap::get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const
{
	minimum_width = natural_width = height;
	Glib::RefPtr<Gdk::Pixbuf> pixbuf;
        load_pixmap(pixbuf, -1, height);
        if (pixbuf) {
                minimum_width = natural_width = pixbuf->get_width();
        }
}
#endif

void DbSelectDialog::WorldMap::on_show(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_show();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

void DbSelectDialog::WorldMap::on_hide(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_hide();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        m_pixbuf.clear();
}

bool DbSelectDialog::WorldMap::on_button_press_event(GdkEventButton * event)
{
        Gdk::Point pt1((int)event->x, (int)event->y);
        Point pt2(screen_to_coord(pt1));
        if (event->button == 1) {
                m_ptrrect = Rect(pt2, pt2);
                m_ptrrect_valid = true;
                redraw();
        }
        std::cerr << "Button Press " << event->button << ": Screen Coordinate: " << pt1.get_x() << ',' << pt1.get_y()
                  << " Lon " << pt2.get_lon_str2() << " Lat " << pt2.get_lat_str2() << std::endl;
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	if (Gtk::DrawingArea::on_button_press_event(event))
		return true;
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        return false;
}

bool DbSelectDialog::WorldMap::on_button_release_event(GdkEventButton * event)
{
        Gdk::Point pt1((int)event->x, (int)event->y);
        Point pt2(screen_to_coord(pt1));
        if (event->button == 1 && m_ptrrect_valid) {
                m_ptrrect = Rect(m_ptrrect.get_southwest(), pt2);
                if (m_ptrrect.get_south() > m_ptrrect.get_north())
                        m_ptrrect = Rect(Point(m_ptrrect.get_west(), m_ptrrect.get_north()),
                                         Point(m_ptrrect.get_east(), m_ptrrect.get_south()));
                signal_rect().emit(m_ptrrect);
                m_ptrrect_valid = false;
                redraw();
        }
        std::cerr << "Button Release " << event->button << ": Screen Coordinate: " << pt1.get_x() << ',' << pt1.get_y()
                  << " Lon " << pt2.get_lon_str2() << " Lat " << pt2.get_lat_str2() << std::endl;
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	if (Gtk::DrawingArea::on_button_release_event(event))
		return true;
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        return false;
}

bool DbSelectDialog::WorldMap::on_motion_notify_event(GdkEventMotion * event)
{
        Gdk::Point pt1((int)event->x, (int)event->y);
        Point pt2(screen_to_coord(pt1));
        m_ptr = pt2;
        m_ptrinside = true;
        if (m_ptrrect_valid)
                m_ptrrect = Rect(m_ptrrect.get_southwest(), pt2);
        redraw();
        std::cerr << "Screen Coordinate: " << pt1.get_x() << ',' << pt1.get_y()
                  << " Lon " << pt2.get_lon_str2() << " Lat " << pt2.get_lat_str2() << std::endl;
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	if (Gtk::DrawingArea::on_motion_notify_event(event))
		return true;
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        return false;
}

bool DbSelectDialog::WorldMap::on_leave_notify_event(GdkEventCrossing * event)
{
        m_ptrinside = false;
        redraw();
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	if (Gtk::DrawingArea::on_leave_notify_event(event))
		return true;
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        return false;
}

Point DbSelectDialog::WorldMap::screen_to_coord(const Gdk::Point & x) const
{
        if (!m_pixbuf)
                return Point(0, 0);
        Point pt;
        pt.set_lon((((int64_t)(x.get_x() - m_offset.get_x())) << 32) / std::max(1, m_pixbuf->get_width()));
        pt.set_lat((((int64_t)(m_offset.get_y() - x.get_y())) << 31) / std::max(1, m_pixbuf->get_height()));
        pt += Point((Point::coord_t)(18.5*Point::from_deg)-(1LL<<31), (1LL<<30));
        pt.set_lat(std::min(std::max(pt.get_lat(), (Point::coord_t)-(1<<30)), (Point::coord_t)(1<<30)));
        return pt;
}

Gdk::Point DbSelectDialog::WorldMap::coord_to_screen(const Point & x) const
{
        if (!m_pixbuf)
                return Gdk::Point(0, 0);
        Point pt1(x - Point((Point::coord_t)(18.5*Point::from_deg)-(1LL<<31), (1LL<<30)));
        Gdk::Point pt;
        pt.set_x(((m_pixbuf->get_width() * ((int64_t)pt1.get_lon() & 0xFFFFFFFFLL)) >> 32) + m_offset.get_x());
        pt.set_y(((m_pixbuf->get_height() * ((int64_t)(-pt1.get_lat()) & 0xFFFFFFFFLL)) >> 31) + m_offset.get_y());
        return pt;
}

DbSelectDialog::DbSelectDialog(void)
        : m_dlg(0), m_dbselectnotebook(0), m_dbselectfindstring(0),
          m_dbselectworldmap(0), m_westlon(0), m_eastlon(0), m_southlat(0), m_northlat(0), m_dbselectregion(0),
          m_rect(Point((Point::coord_t)(-5*Point::from_deg), (Point::coord_t)(20*Point::from_deg)),
                 Point((Point::coord_t)(30*Point::from_deg), (Point::coord_t)(60*Point::from_deg)))
{
        m_timefrom = m_timeto = time(0);
        m_refxml = load_glade_file("dbeditor" UIEXT, "dbselectdialog");
        get_widget(m_refxml, "dbselectdialog", m_dlg);
        get_widget(m_refxml, "dbselectnotebook", m_dbselectnotebook);
        get_widget(m_refxml, "dbselectfindstring", m_dbselectfindstring);
        get_widget(m_refxml, "dbselectfindcomp", m_dbselectfindcomp);
        get_widget_derived(m_refxml, "dbselectworldmap", m_dbselectworldmap);
        get_widget(m_refxml, "dbselectwestlon", m_westlon);
        get_widget(m_refxml, "dbselecteastlon", m_eastlon);
        get_widget(m_refxml, "dbselectsouthlat", m_southlat);
        get_widget(m_refxml, "dbselectnorthlat", m_northlat);
        get_widget(m_refxml, "dbselectregion", m_dbselectregion);
        get_widget(m_refxml, "dbselecttimefrom", m_dbselecttimefrom);
        get_widget(m_refxml, "dbselecttimeto", m_dbselecttimeto);
        Gtk::Button *buttoncancel(0), *buttonok(0);
        Gtk::Button *dbselecttimefromnow(0), *dbselecttimefromday(0), *dbselecttimefromweek(0), *dbselecttimefrommonth(0), *dbselecttimefromyear(0);
        Gtk::Button *dbselecttimetonow(0), *dbselecttimetoday(0), *dbselecttimetoweek(0), *dbselecttimetomonth(0), *dbselecttimetoyear(0);
        get_widget(m_refxml, "dbselectbuttoncancel", buttoncancel);
        get_widget(m_refxml, "dbselectbuttonok", buttonok);
        get_widget(m_refxml, "dbselecttimefromnow", dbselecttimefromnow);
        get_widget(m_refxml, "dbselecttimefromday", dbselecttimefromday);
        get_widget(m_refxml, "dbselecttimefromweek", dbselecttimefromweek);
        get_widget(m_refxml, "dbselecttimefrommonth", dbselecttimefrommonth);
        get_widget(m_refxml, "dbselecttimefromyear", dbselecttimefromyear);
        get_widget(m_refxml, "dbselecttimetonow", dbselecttimetonow);
        get_widget(m_refxml, "dbselecttimetoday", dbselecttimetoday);
        get_widget(m_refxml, "dbselecttimetoweek", dbselecttimetoweek);
        get_widget(m_refxml, "dbselecttimetomonth", dbselecttimetomonth);
        get_widget(m_refxml, "dbselecttimetoyear", dbselecttimetoyear);
        buttoncancel->signal_clicked().connect(sigc::mem_fun(*this, &DbSelectDialog::button_cancel_clicked));
        buttonok->signal_clicked().connect(sigc::mem_fun(*this, &DbSelectDialog::button_ok_clicked));
        m_westlon->signal_changed().connect(sigc::mem_fun(*this, &DbSelectDialog::westlon_changed));
        m_westlon->signal_focus_out_event().connect(sigc::mem_fun(*this, &DbSelectDialog::latlon_focus_out_event));
        m_eastlon->signal_changed().connect(sigc::mem_fun(*this, &DbSelectDialog::eastlon_changed));
        m_eastlon->signal_focus_out_event().connect(sigc::mem_fun(*this, &DbSelectDialog::latlon_focus_out_event));
        m_southlat->signal_changed().connect(sigc::mem_fun(*this, &DbSelectDialog::southlat_changed));
        m_southlat->signal_focus_out_event().connect(sigc::mem_fun(*this, &DbSelectDialog::latlon_focus_out_event));
        m_northlat->signal_changed().connect(sigc::mem_fun(*this, &DbSelectDialog::northlat_changed));
        m_northlat->signal_focus_out_event().connect(sigc::mem_fun(*this, &DbSelectDialog::latlon_focus_out_event));
        m_dlg->signal_delete_event().connect(sigc::mem_fun(*this, &DbSelectDialog::dialog_delete_event));
        m_dbselectworldmap->signal_rect().connect(sigc::mem_fun(*this, &DbSelectDialog::new_pointer_rect));
        m_dbselectregion->signal_changed().connect(sigc::mem_fun(*this, &DbSelectDialog::region_changed));
        dbselecttimefromnow->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &DbSelectDialog::timefrom_set), 0));
        dbselecttimefromday->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &DbSelectDialog::timefrom_set), 24*60*60));
        dbselecttimefromweek->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &DbSelectDialog::timefrom_set), 7*24*60*60));
        dbselecttimefrommonth->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &DbSelectDialog::timefrom_set), 31*24*60*60));
        dbselecttimefromyear->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &DbSelectDialog::timefrom_set), 365*24*60*60));
        dbselecttimetonow->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &DbSelectDialog::timeto_set), 0));
        dbselecttimetoday->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &DbSelectDialog::timeto_set), 24*60*60));
        dbselecttimetoweek->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &DbSelectDialog::timeto_set), 7*24*60*60));
        dbselecttimetomonth->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &DbSelectDialog::timeto_set), 31*24*60*60));
        dbselecttimetoyear->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &DbSelectDialog::timeto_set), 365*24*60*60));
        m_dbselecttimefrom->signal_focus_out_event().connect(sigc::mem_fun(*this, &DbSelectDialog::timefrom_focus_out_event));
        m_dbselecttimefrom->signal_changed().connect(sigc::mem_fun(*this, &DbSelectDialog::timefrom_changed));
        m_dbselecttimeto->signal_focus_out_event().connect(sigc::mem_fun(*this, &DbSelectDialog::timeto_focus_out_event));
        m_dbselecttimeto->signal_changed().connect(sigc::mem_fun(*this, &DbSelectDialog::timeto_changed));
        m_dbselecttimefrom->set_text(Conversions::time_str(m_timefrom));
        m_dbselecttimeto->set_text(Conversions::time_str(m_timeto));
        // setup region model
        m_region_model = Gtk::ListStore::create(m_region_model_columns);
        {
                Gtk::TreeModel::Row row(*(m_region_model->append()));
                row[m_region_model_columns.m_text] = "Central Europe";
                row[m_region_model_columns.m_rect] = Rect(Point((Point::coord_t)(-5.0 * Point::from_deg), (Point::coord_t)(40.0 * Point::from_deg)),
                                                          Point((Point::coord_t)(15.0 * Point::from_deg), (Point::coord_t)(60.0 * Point::from_deg)));
                row = *(m_region_model->append());
                row[m_region_model_columns.m_text] = "Europe";
                row[m_region_model_columns.m_rect] = Rect(Point((Point::coord_t)(-10.0 * Point::from_deg), (Point::coord_t)(20.0 * Point::from_deg)),
                                                          Point((Point::coord_t)(35.0 * Point::from_deg), (Point::coord_t)(80.0 * Point::from_deg)));
        }
        m_dbselectregion->clear();
        m_dbselectregion->set_model(m_region_model);
        {
                Gtk::CellRendererText *text_renderer = Gtk::manage(new Gtk::CellRendererText());
                text_renderer->set_property("xalign", 0.0);
                m_dbselectregion->pack_start(*text_renderer, true);
                m_dbselectregion->add_attribute(*text_renderer, "text", m_region_model_columns.m_text);
        }
        m_dbselectregion->set_active(0);
        m_comp_model = Gtk::ListStore::create(m_comp_model_columns);
        {
                static const DbQueryInterfaceCommon::comp_t comps[] = {
                        DbQueryInterfaceCommon::comp_startswith,
                        DbQueryInterfaceCommon::comp_contains,
                        DbQueryInterfaceCommon::comp_exact,
                        DbQueryInterfaceCommon::comp_exact_casesensitive,
                        DbQueryInterfaceCommon::comp_like
                };
                static const char *strs[] = {
                        "Starts With",
                        "Contains",
                        "Exact (case insensitive)",
                        "Exact (case sensitive)",
                        "Like (wildcards % and _)"
                };
                for (unsigned int i = 0; i < sizeof(comps)/sizeof(comps[0]); i++) {
                        Gtk::TreeModel::Row row(*(m_comp_model->append()));
                        row[m_comp_model_columns.m_text] = strs[i];
                        row[m_comp_model_columns.m_comp] = comps[i];
                }
        }
        m_dbselectfindcomp->clear();
        m_dbselectfindcomp->set_model(m_comp_model);
        {
                Gtk::CellRendererText *comp_renderer = Gtk::manage(new Gtk::CellRendererText());
                comp_renderer->set_property("xalign", 0.0);
                m_dbselectfindcomp->pack_start(*comp_renderer, true);
                m_dbselectfindcomp->add_attribute(*comp_renderer, "text", m_comp_model_columns.m_text);
        }
        m_dbselectfindcomp->set_active(1);
        rect_changed();
        timefrom_set(0);
        timeto_set(0);
}

DbSelectDialog::~DbSelectDialog(void)
{
}

void DbSelectDialog::show(bool doit)
{
        if (doit)
                m_dlg->show();
        else
                m_dlg->hide();
}

bool DbSelectDialog::dialog_delete_event(GdkEventAny * event)
{
        signal_cancel().emit();
        return false;
}

void DbSelectDialog::button_cancel_clicked(void)
{
        m_dlg->hide();
        signal_cancel().emit();
}

void DbSelectDialog::button_ok_clicked(void)
{
        int pgnr = m_dbselectnotebook->get_current_page();
        m_dlg->hide();
        switch (pgnr) {
                case 0:
                {
                        Gtk::TreeModel::iterator iter(m_dbselectfindcomp->get_active());
                        DbQueryInterfaceCommon::comp_t comp(DbQueryInterfaceCommon::comp_contains);
                        if (iter)
                                comp = (*iter)[m_comp_model_columns.m_comp];
                        signal_byname().emit(m_dbselectfindstring->get_text(), comp);
                        break;
                }

                case 1:
                        signal_byrectangle().emit(m_rect);
                        break;

                case 2:
                        //std::cerr << "Select time from: " << Conversions::time_str("%Y-%m-%d %H:%M:%S", m_timefrom)
                        //                << " to: " << Conversions::time_str("%Y-%m-%d %H:%M:%S", m_timeto) << std::endl;
                        signal_bytime().emit(m_timefrom, m_timeto);
                        break;

                default:
                        std::cerr << "DbSelectDialog: unknown notebook page " << pgnr << std::endl;
                        signal_cancel().emit();
                        break;
        }
}

void DbSelectDialog::westlon_changed(void)
{
        Point pt(m_rect.get_southwest());
        if (pt.set_lon_str(m_westlon->get_text())) {
                m_rect = Rect(pt, m_rect.get_northeast());
                m_dbselectregion->set_active(-1);
        }
        //rect_changed();
}

void DbSelectDialog::eastlon_changed(void)
{
        Point pt(m_rect.get_northeast());
        if (pt.set_lon_str(m_eastlon->get_text())) {
                m_rect = Rect(m_rect.get_southwest(), pt);
                m_dbselectregion->set_active(-1);
        }
        //rect_changed();
}

void DbSelectDialog::southlat_changed(void)
{
        Point pt(m_rect.get_southwest());
        if (pt.set_lat_str(m_southlat->get_text())) {
                m_rect = Rect(pt, m_rect.get_northeast());
                m_dbselectregion->set_active(-1);
        }
        //rect_changed();
}

void DbSelectDialog::northlat_changed(void)
{
        Point pt(m_rect.get_northeast());
        //std::cerr << "northlat_changed: " << m_northlat->get_text() << std::endl;
        if (pt.set_lat_str(m_northlat->get_text())) {
                m_rect = Rect(m_rect.get_southwest(), pt);
                m_dbselectregion->set_active(-1);
        }
        //rect_changed();
}

bool DbSelectDialog::latlon_focus_out_event(GdkEventFocus * event)
{
        rect_changed();
        return false;
}

void DbSelectDialog::rect_changed(void)
{
        m_westlon->set_text(m_rect.get_southwest().get_lon_str());
        m_eastlon->set_text(m_rect.get_northeast().get_lon_str());
        m_southlat->set_text(m_rect.get_southwest().get_lat_str());
        m_northlat->set_text(m_rect.get_northeast().get_lat_str());
        m_dbselectworldmap->set_rect(m_rect);
}

void DbSelectDialog::new_pointer_rect(const Rect & r)
{
        m_rect = r;
        m_dbselectregion->set_active(-1);
        rect_changed();
}

void DbSelectDialog::region_changed(void)
{
        Gtk::TreeModel::iterator iter = m_dbselectregion->get_active();
        if (!iter)
                return;
        Gtk::TreeModel::Row row(*iter);
        m_rect = row[m_region_model_columns.m_rect];
        rect_changed();
}

void DbSelectDialog::timefrom_changed(void)
{
        m_timefrom = Conversions::time_parse(m_dbselecttimefrom->get_text());
}

bool DbSelectDialog::timefrom_focus_out_event(GdkEventFocus * event)
{
        timefrom_changed();
        m_dbselecttimefrom->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", m_timefrom));
        return false;
}

void DbSelectDialog::timeto_changed(void)
{
        m_timeto = Conversions::time_parse(m_dbselecttimeto->get_text());
}

bool DbSelectDialog::timeto_focus_out_event(GdkEventFocus * event)
{
        timeto_changed();
        m_dbselecttimeto->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", m_timeto));
        return false;
}

void DbSelectDialog::timefrom_set(time_t offs)
{
        m_timefrom = time(0) - offs;
        m_dbselecttimefrom->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", m_timefrom));
}

void DbSelectDialog::timeto_set(time_t offs)
{
        m_timeto = time(0) - offs;
        m_dbselecttimeto->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", m_timeto));
}

template<class T> UndoRedoStack<T>::UndoRedoStack(void)
        : m_stack(), m_stackptr(0)
{
}

template<class T> void UndoRedoStack<T>::limit_stack(void)
{
        if (m_stack.size() < max_undo)
                return;
        unsigned int nrdel = m_stack.size() - max_undo + 1;
        if (m_stackptr <= nrdel)
                m_stackptr = 0;
        else
                m_stackptr -= nrdel;
        m_stack.erase(m_stack.begin(), m_stack.begin() + nrdel);
}

template<class T> void UndoRedoStack<T>::save(const T & e)
{
        limit_stack();
        if (m_stackptr < m_stack.size())
                m_stack.resize(m_stackptr);
        m_stack.push_back(e);
        m_stackptr = m_stack.size();
}

template<class T> void UndoRedoStack<T>::erase(const element_t & e)
{
        limit_stack();
        if (m_stackptr < m_stack.size())
                m_stack.resize(m_stackptr);
        m_stack.push_back(e);
        m_stackptr = m_stack.size();
}

template<class T> bool UndoRedoStack<T>::can_undo(void) const
{
        return m_stackptr > 1;
}

template<class T> T UndoRedoStack<T>::undo(void)
{
        if (!can_undo())
                return T();
        m_stackptr--;
        return m_stack[m_stackptr - 1];
}

template<class T> bool UndoRedoStack<T>::can_redo(void) const
{
        return m_stackptr < m_stack.size();
}

template<class T> T UndoRedoStack<T>::redo(void)
{
        if (!can_redo())
                return T();
        m_stackptr++;
        return m_stack[m_stackptr - 1];
}

template class UndoRedoStack<NavaidsDb::Navaid>;
template class UndoRedoStack<WaypointsDb::Waypoint>;
template class UndoRedoStack<AirwaysDb::Airway>;
template class UndoRedoStack<AirspacesDb::Airspace>;
template class UndoRedoStack<AirportsDb::Airport>;
template class UndoRedoStack<TracksDb::Track>;

DbEditor::DbEditorWindow::DbEditorWindow(BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml)
        : toplevel_window_t(castitem), m_fullscreen(false)
{
        signal_window_state_event().connect(sigc::mem_fun(*this, &DbEditor::DbEditorWindow::on_my_window_state_event));
        signal_delete_event().connect(sigc::mem_fun(*this, &DbEditor::DbEditorWindow::on_my_delete_event));
        signal_key_press_event().connect(sigc::mem_fun(*this, &DbEditor::DbEditorWindow::on_my_key_press_event));
}

DbEditor::DbEditorWindow::~DbEditorWindow()
{
}

void DbEditor::DbEditorWindow::toggle_fullscreen(void)
{
        if (m_fullscreen)
                unfullscreen();
        else
                fullscreen();
}

bool DbEditor::DbEditorWindow::on_my_window_state_event(GdkEventWindowState *state)
{
        if (!state)
                return false;
        m_fullscreen = !!(Gdk::WINDOW_STATE_FULLSCREEN & state->new_window_state);
        return false;
}

bool DbEditor::DbEditorWindow::on_my_delete_event(GdkEventAny *event)
{
        hide();
        return true;
}

bool DbEditor::DbEditorWindow::on_my_key_press_event(GdkEventKey * event)
{
        if(!event)
                return false;
        std::cerr << "key press: type " << event->type << " keyval " << event->keyval << std::endl;
        if (event->type != GDK_KEY_PRESS)
                return false;
        switch (event->keyval) {
                case GDK_KEY_F6:
                        toggle_fullscreen();
                        return true;

                case GDK_KEY_F7:
                        return zoom_in();

                case GDK_KEY_F8:
                        return zoom_out();

                default:
                        break;
        }
        return false;
}

DbEditor::TopoEngine::TopoEngine(const std::string & dir_main, const std::string & dir_aux)
{
        try {
                m_topodb.open(dir_aux.empty() ? std::string(PACKAGE_DATA_DIR) : dir_aux);
        } catch (const std::exception& e) {
                std::cerr << "Error opening topo database: " << e.what() << std::endl;
        }
#ifdef HAVE_PILOTLINK
        try {
                m_palmgtopodb.open(PACKAGE_DATA_DIR "/GTopoDB.pdb");
        } catch (const std::exception& e) {
                std::cerr << "Error opening palm gtopo database: " << e.what() << std::endl;
        }
#endif
}

TopoDb30::minmax_elev_t DbEditor::TopoEngine::get_minmax_elev(const Rect & re)
{
#ifdef HAVE_PILOTLINK
        if (!m_palmgtopodb.is_open())
                return m_topodb.get_minmax_elev(re);
        PalmGtopo::minmax_elevation_t mm(m_palmgtopodb.get_minmax_elev(re));
        TopoDb30::minmax_elev_t r;
        r.first = mm.first;
        r.second = mm.second;
        if (mm.first == PalmGtopo::nodata)
                r.first = TopoDb30::nodata;
        if (mm.second == PalmGtopo::nodata)
                r.second = TopoDb30::nodata;
        return r;
#else
        return m_topodb.get_minmax_elev(re);
#endif
}

TopoDb30::minmax_elev_t DbEditor::TopoEngine::get_minmax_elev(const PolygonSimple& p)
{
#ifdef HAVE_PILOTLINK
        if (!m_palmgtopodb.is_open())
                return m_topodb.get_minmax_elev(p);
        PalmGtopo::minmax_elevation_t mm(m_palmgtopodb.get_minmax_elev(p));
        TopoDb30::minmax_elev_t r;
        r.first = mm.first;
        r.second = mm.second;
        if (mm.first == PalmGtopo::nodata)
                r.first = TopoDb30::nodata;
        if (mm.second == PalmGtopo::nodata)
                r.second = TopoDb30::nodata;
        return r;
#else
        return m_topodb.get_minmax_elev(p);
#endif
}
