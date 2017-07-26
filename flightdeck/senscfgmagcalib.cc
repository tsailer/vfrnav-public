//
// C++ Implementation: Sensor Configuration Dialog
//
// Description: Sensor Configuration Dialog
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <iomanip>
#include <glibmm.h>
#include <gsl/gsl_multifit_nlin.h>

#include "flightdeckwindow.h"

class FlightDeckWindow::MagCalibDialog::Graph : public Gtk::DrawingArea {
public:
	explicit Graph(BaseObjectType* cobject, const Glib::RefPtr<Builder>& builder);
	virtual ~Graph();

	void set_ellipses(void);
	void set_ellipses(const Eigen::Vector3d& center, const Eigen::Vector3d& radius);
	void set_data(const MagCalibDialog::measurements_t *data);
	void update(void);

protected:
	static const double tracecol[3][3];
	Eigen::Vector3d m_ellipcenter;
	Eigen::Vector3d m_ellipradius;
	Cairo::RefPtr<Cairo::Surface> m_pointcloud;
	const measurements_t *m_data;
	unsigned int m_dataptr;
	bool m_drawellipses;

	virtual void on_size_allocate(Gtk::Allocation& allocation);
#ifdef HAVE_GTKMM2
	virtual bool on_expose_event(GdkEventExpose *event);
#endif
	virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);

	static inline void set_trace_color(const Cairo::RefPtr<Cairo::Context>& cr, unsigned int idx, double alpha = 0.5) {
		idx = std::min(idx, 2U);
		cr->set_source_rgba(tracecol[idx][0], tracecol[idx][1], tracecol[idx][2], alpha);
	}
};

const double FlightDeckWindow::MagCalibDialog::Graph::tracecol[3][3] = {
	{ 0.0, 1.0, 1.0 },
	{ 1.0, 0.0, 1.0 },
	{ 1.0, 1.0, 0.0 }
};

FlightDeckWindow::MagCalibDialog::Graph::Graph(BaseObjectType* cobject, const Glib::RefPtr<Builder>& builder)
	: Gtk::DrawingArea(cobject), m_ellipcenter(0, 0, 0), m_ellipradius(0, 0, 0), m_data(0), m_dataptr(0), m_drawellipses(false)
{
}

FlightDeckWindow::MagCalibDialog::Graph::~Graph()
{
}

void FlightDeckWindow::MagCalibDialog::Graph::set_ellipses(void)
{
	m_drawellipses = false;
}

void FlightDeckWindow::MagCalibDialog::Graph::set_ellipses(const Eigen::Vector3d& center, const Eigen::Vector3d& radius)
{
	m_ellipcenter = center;
	m_ellipradius = radius;
	m_drawellipses = true;
}

void FlightDeckWindow::MagCalibDialog::Graph::set_data(const MagCalibDialog::measurements_t *data)
{
	m_data = data;
	m_pointcloud.clear();
	m_dataptr = 0;
}

void FlightDeckWindow::MagCalibDialog::Graph::update(void)
{
	if (!m_data || m_dataptr > m_data->size()) {
		m_pointcloud.clear();
		m_dataptr = 0;
	}	
	queue_draw();
}

void FlightDeckWindow::MagCalibDialog::Graph::on_size_allocate(Gtk::Allocation& allocation)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_size_allocate(allocation);
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

#ifdef HAVE_GTKMM2
bool FlightDeckWindow::MagCalibDialog::Graph::on_expose_event(GdkEventExpose *event)
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
#endif

bool FlightDeckWindow::MagCalibDialog::Graph::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	if (!cr)
		return true;
	cr->set_source_rgb(1, 1, 1);
	cr->paint();
       	{
		double vmax(0);
		if (m_data) {
			for (MagCalibDialog::measurements_t::const_iterator di(m_data->begin()), de(m_data->end()); di != de; ++di)
				vmax = std::max(vmax, di->array().abs().maxCoeff());
			vmax *= 1.125;
		}
		if (vmax <= 0)
			vmax = 32768;
		double x1, y1, x2, y2;
		cr->get_clip_extents(x1, y1, x2, y2);
		double scale(std::min(x2 - x1, y2 - y1));
		scale *= 0.5 / vmax;
		cr->translate(0.5 * (x1 + x2), 0.5 * (y1 + y2));
		cr->scale(scale, scale);
	}
	cr->set_source_rgb(0, 0, 0);
	cr->set_line_width(50);
	cr->move_to(-32768, 0);
	cr->line_to(32768, 0);
	cr->move_to(0, -32768);
	cr->line_to(0, 32768);
	cr->stroke();
	if (m_drawellipses) {
		for (int i = 0; i < 3; ++i) {
			int j = (i + 1) % 3;
			set_trace_color(cr, i);
			cr->save();
			cr->translate(m_ellipcenter[i], m_ellipcenter[j]);
			cr->scale(m_ellipradius[i], m_ellipradius[j]);
			cr->arc(0, 0, 1, 0, 2*M_PI);
			cr->restore();
			cr->stroke();
		}			
	}
	if (false) {
		if (!m_data) {
			m_pointcloud.clear();
			m_dataptr = 0;
		} else {
			if (m_dataptr > m_data->size()) {
				m_pointcloud.clear();
				m_dataptr = 0;
			}
			if (!m_pointcloud)
				m_pointcloud = Cairo::Surface::create(cr->get_target(), Cairo::CONTENT_COLOR_ALPHA, 2048, 2048);
			Cairo::RefPtr<Cairo::Context> ctx(Cairo::Context::create(m_pointcloud));
			ctx->translate(1024, 1024);
			ctx->scale(1.0 / 32, 1.0 / 32);
			Cairo::RefPtr<Cairo::Pattern> pointpt[3];
			for (int i = 0; i < 3; ++i) {
				ctx->push_group();
				set_trace_color(ctx, i);
				ctx->arc(0, 0, 200, 0, 2*M_PI);
				ctx->fill();
				pointpt[i] = ctx->pop_group();
			}
			for (unsigned int sz = m_data->size(); m_dataptr < sz; ++m_dataptr) {
				const MagCalibDialog::magmeas_t& mag((*m_data)[m_dataptr]);
				int i = 0;
				if (abs(mag(1)) < abs(mag(0)))
					i = 1;
				if (abs(mag(2)) < abs(mag(i)))
					i = 2;
				i = (i + 1) % 3;
				int j = (i + 1) % 3;
				//ctx->set_source(pointpt[i]);
				set_trace_color(ctx, i);
				ctx->save();
				ctx->translate(mag(i), mag(j));
				ctx->rectangle(-50, -50, 100, 100);
				ctx->fill();
				ctx->restore();
			}
		}
		if (m_pointcloud) {
			cr->save();
			cr->scale(32, 32);
			cr->set_source(m_pointcloud, -32768, -32768);
			cr->paint();
			cr->restore();
		}
	}
	return true;
}

FlightDeckWindow::MagCalibDialog::MagCalibDialog(BaseObjectType* cobject, const Glib::RefPtr<Builder>& builder)
	: Gtk::Dialog(cobject), m_buttonapply(0), m_buttonclose(0), m_graph(0), m_nrmeas(0),
	  m_vmagbias(0, 0, 0), m_vmagscale(1e-4, 1e-4, 1e-4), m_div(0), m_calrun(false)
{
	builder->get_widget("mcbuttonapply", m_buttonapply);
	builder->get_widget("mcbuttonclose", m_buttonclose);
	builder->get_widget_derived("mcgraph", m_graph);
	builder->get_widget("mcnrmeas", m_nrmeas);
	for (int i = 0; i < 3; ++i) {
		m_mag[i] = m_magbias[i] = m_magscale[i] = m_oldmagbias[i] = m_oldmagscale[i] = 0;
		std::string suffix(1, 'x' + i);
		builder->get_widget("mcmag" + suffix, m_mag[i]);
		builder->get_widget("mcmagbias" + suffix, m_magbias[i]);
		builder->get_widget("mcmagscale" + suffix, m_magscale[i]);
		builder->get_widget("mcoldmagbias" + suffix, m_oldmagbias[i]);
		builder->get_widget("mcoldmagscale" + suffix, m_oldmagscale[i]);
	}
	if (m_buttonapply)
		m_buttonapply->signal_clicked().connect(sigc::mem_fun(*this, &MagCalibDialog::button_apply));
	if (m_buttonclose)
		m_buttonclose->signal_clicked().connect(sigc::mem_fun(*this, &MagCalibDialog::button_close));
	init();
	if (m_graph)
		m_graph->set_data(&m_measurements);
}

FlightDeckWindow::MagCalibDialog::~MagCalibDialog()
{
}

void FlightDeckWindow::MagCalibDialog::init(const Eigen::Vector3d& oldmagbias, const Eigen::Vector3d& oldmagscale)
{
	init();
	for (int i = 0; i < 3; ++i) {
		if (m_oldmagbias[i] && !std::isnan(oldmagbias(i))) {
			std::ostringstream oss;
			oss << oldmagbias(i);
			m_oldmagbias[i]->set_text(oss.str());
			m_oldmagbias[i]->set_sensitive(true);
		}
		if (m_oldmagscale[i] && !std::isnan(oldmagscale(i))) {
			std::ostringstream oss;
			oss << oldmagscale(i);
			m_oldmagscale[i]->set_text(oss.str());
			m_oldmagscale[i]->set_sensitive(true);
		}
	}
}

void FlightDeckWindow::MagCalibDialog::init(void)
{
	m_calrun = false;
	m_div = 0;
        m_measurements.clear();
	m_minmeas = Eigen::Vector3d::Constant(std::numeric_limits<double>::max());
	m_maxmeas = Eigen::Vector3d::Constant(std::numeric_limits<double>::min());
	m_vmagbias = Eigen::Vector3d::Zero();
	m_vmagscale = Eigen::Vector3d::Constant(1e-4);
	for (int i = 0; i < 3; ++i) {
		if (m_mag[i])
			m_mag[i]->set_text("");
		if (m_magbias[i]) {
			m_magbias[i]->set_text("");
			m_magbias[i]->set_sensitive(false);
		}
		if (m_magscale[i]) {
			m_magscale[i]->set_text("");
			m_magscale[i]->set_sensitive(false);
		}
		if (m_oldmagbias[i]) {
			m_oldmagbias[i]->set_text("");
			m_oldmagbias[i]->set_sensitive(false);
		}
		if (m_oldmagscale[i]) {
			m_oldmagscale[i]->set_text("");
			m_oldmagscale[i]->set_sensitive(false);
		}
	}
	if (m_nrmeas)
		m_nrmeas->set_text("0");
	if (m_buttonapply)
		m_buttonapply->set_sensitive(false);
	if (m_graph) {
		m_graph->set_ellipses();
		m_graph->update();
	}
}

void FlightDeckWindow::MagCalibDialog::start(void)
{
        m_calrun = true;
}

void FlightDeckWindow::MagCalibDialog::button_apply(void)
{
	m_result.emit(m_vmagbias, m_vmagscale);
	button_close();
}

void FlightDeckWindow::MagCalibDialog::button_close(void)
{
	init();
	hide();
}

int FlightDeckWindow::MagCalibDialog::magsolve_f(const gsl_vector *x, void *data, gsl_vector *f)
{
	if (!data)
		return GSL_EINVAL;
	return static_cast<MagCalibDialog *>(data)->magsolve_func(x, f, 0);
}

int FlightDeckWindow::MagCalibDialog::magsolve_df(const gsl_vector *x, void *data, gsl_matrix *J)
{
	if (!data)
		return GSL_EINVAL;
	return static_cast<MagCalibDialog *>(data)->magsolve_func(x, 0, J);
}

int FlightDeckWindow::MagCalibDialog::magsolve_fdf(const gsl_vector *x, void *data, gsl_vector *f, gsl_matrix *J)
{
	if (!data)
		return GSL_EINVAL;
	return static_cast<MagCalibDialog *>(data)->magsolve_func(x, f, J);
}

int FlightDeckWindow::MagCalibDialog::magsolve_func(const gsl_vector *x, gsl_vector *f, gsl_matrix *J) const
{
	if (!x)
		return GSL_EINVAL;
	if (!f && !J)
		return GSL_SUCCESS;
	for (int i = 0; i < (int)m_measurements.size(); ++i) {
		double ff(0);
		const magmeas_t& mag(m_measurements[i]);
		for (int j = 0; j < 3; ++j) {
			double t(mag(j) - gsl_vector_get(x, j+3));
			t *= gsl_vector_get(x, j);
			ff += t * t;
		}
		ff = sqrt(ff);
		if (f)
			gsl_vector_set(f, i, ff - 1);
		if (!J)
			continue;
		double ffinv(1.0 / ff);
		for (int j = 0; j < 3; ++j) {
			double t(gsl_vector_get(x, j + 3) - mag(j));
			double s(gsl_vector_get(x, j));
			gsl_matrix_set(J, i, j, ffinv * s * t * t);
			gsl_matrix_set(J, i, j + 3, ffinv * s * s * t);
		}
	}
	return GSL_SUCCESS;	
}

void FlightDeckWindow::MagCalibDialog::newrawvalue(const Eigen::Vector3d& mag)
{
	static const double minanglediff = 10;
	static const double anglethr = cos(minanglediff * M_PI / 180.0);
	++m_div;
	if (!(m_div & 3)) {
		for (int i = 0; i < 3; ++i) {
			if (m_mag[i]) {
				std::ostringstream oss;
				oss << mag(i);
				m_mag[i]->set_text(oss.str());
			}
		}
	}
	if (!m_calrun)
		return;
	if ((mag.array() == 0).all())
		return;
	{
		bool ok(m_measurements.empty());
		if (!ok) {
			const Eigen::Vector3d& u(m_measurements.back());
			if (false) {
				double ca(std::min(u.dot(mag) / (u.norm() * mag.norm()), 1.0));
				ok = ca < anglethr;
			} else {
				double ca((u - mag).norm() / u.norm());
				ok = ca > 0.1;
			}
		}
		if (!ok)
			return;
		if (m_measurements.size() >= 1000000)
			return;
		m_measurements.push_back(mag);
		m_minmeas = m_minmeas.array().min(mag.array());
		m_maxmeas = m_maxmeas.array().max(mag.array());
	}
	if (m_nrmeas) {
		std::ostringstream oss;
		oss << m_measurements.size();
		m_nrmeas->set_text(oss.str());
	}
	if (m_measurements.size() < 32) {
		if (m_graph) {
			m_graph->set_ellipses();
			m_graph->update();
		}
		return;
	}
	// check if the estimate is too far off
	if ((m_vmagbias.array() > m_maxmeas.array()).any() ||
	    (m_vmagbias.array() < m_minmeas.array()).any() ||
	    (m_vmagscale.array() > 0.1).any() ||
	    (m_vmagscale.array() < 5e-5).any()) {
		m_vmagbias = 0.5 * (m_minmeas +m_maxmeas );
		m_vmagscale = Eigen::Vector3d::Constant(3e-4);
	}
	{
		gsl_multifit_function_fdf f;
		f.f = &MagCalibDialog::magsolve_f;
		f.df = &MagCalibDialog::magsolve_df;
		f.fdf = &MagCalibDialog::magsolve_fdf;
		f.n = m_measurements.size();
		f.p = 6;
		f.params = this;
		const gsl_multifit_fdfsolver_type *T = gsl_multifit_fdfsolver_lmsder;
		gsl_multifit_fdfsolver *s = gsl_multifit_fdfsolver_alloc(T, f.n, f.p);
		{
			Eigen::Matrix<double, 6, 1> x_init;
			x_init.block<3, 1>(0, 0) = m_vmagscale;
			x_init.block<3, 1>(3, 0) = m_vmagbias;
			gsl_vector_const_view x = gsl_vector_const_view_array(x_init.data(), 6);
			gsl_multifit_fdfsolver_set(s, &f, &x.vector);
		}
		unsigned int iter(0);
		int status;
		do {
			++iter;
			// check state is in reasonable shape
			for (int i = 0; i < 3; ++i) {
				if (gsl_vector_get(s->x, i) < 4.99e-5)
					gsl_vector_set(s->x, i, 4.99e-5);
				double b(gsl_vector_get(s->x, i + 3));
				if (b < m_minmeas(i))
					gsl_vector_set(s->x, i + 3, m_minmeas(i));
				else if (b > m_maxmeas(i))
					gsl_vector_set(s->x, i + 3, m_maxmeas(i));
			}
			status = gsl_multifit_fdfsolver_iterate(s);
			if (true)
				std::cerr << "Mag Solve: iter " << iter << " status " << gsl_strerror(status) << std::endl;
			if (status)
				break;
			status = gsl_multifit_test_delta(s->dx, s->x, 1e-4, 1e-4);
		} while (status == GSL_CONTINUE && iter < 64);
		for (int i = 0; i < 3; ++i) {
			m_vmagscale(i) = gsl_vector_get(s->x, i);
			m_vmagbias(i) = gsl_vector_get(s->x, i + 3);
		}
		gsl_multifit_fdfsolver_free(s);
	}
	// Sanitize output
	m_vmagscale = abs(m_vmagscale.array());
	for (int i = 0; i < 3; ++i) {
		if (m_magbias[i]) {
			std::ostringstream oss;
			oss << m_vmagbias(i);
			m_magbias[i]->set_text(oss.str());
			m_magbias[i]->set_sensitive(true);
		}
		if (m_magscale[i]) {
			std::ostringstream oss;
			oss << m_vmagscale(i);
			m_magscale[i]->set_text(oss.str());
			m_magscale[i]->set_sensitive(true);
		}
	}
	if (m_buttonapply) {
		bool err((m_vmagbias.array() > m_maxmeas.array()).any() ||
			 (m_vmagbias.array() < m_minmeas.array()).any() ||
			 (m_vmagscale.array() > 0.1).any() ||
			 (m_vmagscale.array() < 5e-5).any());
		m_buttonapply->set_sensitive(!err);
	}
	if (m_graph) {
		if ((m_vmagscale.array() > 0).all())
			m_graph->set_ellipses(m_vmagbias, Eigen::Array3d::Constant(1.0) / m_vmagscale.array());
		else
			m_graph->set_ellipses();
		m_graph->update();
	}
}
