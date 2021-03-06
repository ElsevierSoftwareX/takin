/**
 * Form Factor & Scattering Length Dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date nov-2015
 * @license GPLv2
 */

#include "FormfactorDlg.h"
#include "libs/formfactors/formfact.h"
#include "tlibs/string/spec_char.h"
#include "tlibs/phys/term.h"
#include "libs/qthelper.h"

using t_real = t_real_glob;


FormfactorDlg::FormfactorDlg(QWidget* pParent, QSettings *pSettings)
	: QDialog(pParent), m_pSettings(pSettings)
{
	this->setupUi(this);
	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") &&
			font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);
	}

	// form factors
	if(g_bHasFormfacts)
	{
		SetupAtoms();

		m_plotwrap.reset(new QwtPlotWrapper(plotF));
		m_plotwrap->GetCurve(0)->setTitle("Atomic Form Factor");
		m_plotwrap->GetPlot()->setAxisTitle(QwtPlot::xBottom, "Scattering Wavenumber Q (1/A)");
		m_plotwrap->GetPlot()->setAxisTitle(QwtPlot::yLeft, "Atomic Form Factor f (e-)");
		if(m_plotwrap->HasTrackerSignal())
			connect(m_plotwrap->GetPicker(), SIGNAL(moved(const QPointF&)),
				this, SLOT(cursorMoved(const QPointF&)));

		// connections
		QObject::connect(listAtoms, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
			this, SLOT(AtomSelected(QListWidgetItem*, QListWidgetItem*)));
		QObject::connect(editFilter, SIGNAL(textEdited(const QString&)),
			this, SLOT(SearchAtom(const QString&)));

		SearchAtom("H");
	}
	else
	{
		tabFF->setEnabled(0);
	}

	// mag. form factors
	if(g_bHasMagFormfacts)
	{
		SetupMagAtoms();

		m_plotwrap_m.reset(new QwtPlotWrapper(plotMF));
		m_plotwrap_m->GetCurve(0)->setTitle("Magnetic Form Factor");
		m_plotwrap_m->GetPlot()->setAxisTitle(QwtPlot::xBottom, "Scattering Wavenumber Q (1/A)");
		m_plotwrap_m->GetPlot()->setAxisTitle(QwtPlot::yLeft, "Magnetic Form Factor f_M");
		if(m_plotwrap_m->HasTrackerSignal())
			connect(m_plotwrap_m->GetPicker(), SIGNAL(moved(const QPointF&)), this, SLOT(cursorMoved(const QPointF&)));

		// connections
		QObject::connect(listMAtoms, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
			this, SLOT(MagAtomSelected(QListWidgetItem*, QListWidgetItem*)));
		QObject::connect(editMFilter, SIGNAL(textEdited(const QString&)),
			this, SLOT(SearchMagAtom(const QString&)));

		for(QDoubleSpinBox* pSpin : {spinL, spinS, spinJ})
			QObject::connect(pSpin, SIGNAL(valueChanged(double)), this, SLOT(Calcg()));
		for(QDoubleSpinBox* pSpin : {sping, spinL, spinS, spinJ})
			QObject::connect(pSpin, SIGNAL(valueChanged(double)), this, SLOT(RefreshMagAtom()));

		QObject::connect(editOrbital, SIGNAL(textEdited(const QString&)),
			this, SLOT(CalcTermSymbol(const QString&)));
	}
	else
	{
		tabMFF->setEnabled(0);
	}


	// scattering lengths
	if(g_bHasScatlens)
	{
		SetupScatteringLengths();

		m_plotwrapSc.reset(new QwtPlotWrapper(plotSc));
		m_plotwrapSc->GetCurve(0)->setTitle("Scattering Lengths");
		m_plotwrapSc->GetPlot()->setAxisTitle(QwtPlot::xBottom, "Element");
		m_plotwrapSc->GetPlot()->setAxisTitle(QwtPlot::yLeft, "Scattering Length b (fm)");
		if(m_plotwrapSc->HasTrackerSignal())
			connect(m_plotwrapSc->GetPicker(), SIGNAL(moved(const QPointF&)), this, SLOT(cursorMoved(const QPointF&)));

		// connections
		QObject::connect(radioCoherent, SIGNAL(toggled(bool)),
			this, SLOT(PlotScatteringLengths()));
		QObject::connect(editSLSearch, SIGNAL(textEdited(const QString&)),
			this, SLOT(SearchSLAtom(const QString&)));

		PlotScatteringLengths();
	}
	else
	{
		tabSc->setEnabled(0);
	}


	if(m_pSettings && m_pSettings->contains("formfactors/geo"))
		restoreGeometry(m_pSettings->value("formfactors/geo").toByteArray());

	//radioCoherent->setChecked(1);
}

FormfactorDlg::~FormfactorDlg()
{}


static QListWidgetItem* create_header_item(const char *pcTitle, bool bSubheader=0)
{
	QListWidgetItem *pHeaderItem = new QListWidgetItem(pcTitle);
	pHeaderItem->setTextAlignment(Qt::AlignHCenter);

	QFont fontHeader = pHeaderItem->font();
	fontHeader.setBold(1);
	pHeaderItem->setFont(fontHeader);

	QBrush brushHeader = pHeaderItem->foreground();
	brushHeader.setColor(QColor(0xff, 0xff, 0xff));
	pHeaderItem->setForeground(brushHeader);

	pHeaderItem->setData(Qt::UserRole, 0);
	pHeaderItem->setBackgroundColor(bSubheader ? QColor(0x85, 0x85, 0x85) : QColor(0x65, 0x65, 0x65));

	return pHeaderItem;
}


void FormfactorDlg::SetupAtoms()
{
	std::shared_ptr<const FormfactList<t_real>> lstff = FormfactList<t_real>::GetInstance();
	listAtoms->addItem(create_header_item("Atoms"));
	for(std::size_t iFF=0; iFF<lstff->GetNumAtoms(); ++iFF)
	{
		if(iFF==0) listAtoms->addItem(create_header_item("Period 1", 1));
		else if(iFF==2) listAtoms->addItem(create_header_item("Period 2", 1));
		else if(iFF==10) listAtoms->addItem(create_header_item("Period 3", 1));
		else if(iFF==18) listAtoms->addItem(create_header_item("Period 4", 1));
		else if(iFF==36) listAtoms->addItem(create_header_item("Period 5", 1));
		else if(iFF==54) listAtoms->addItem(create_header_item("Period 6", 1));
		else if(iFF==86) listAtoms->addItem(create_header_item("Period 7", 1));

		const Formfact<t_real>& ff = lstff->GetAtom(iFF);
		const std::string& strAtom = ff.GetAtomIdent();

		std::ostringstream ostrAtom;
		ostrAtom << (iFF+1) << " " << strAtom;
		QListWidgetItem* pItem = new QListWidgetItem(ostrAtom.str().c_str());
		pItem->setData(Qt::UserRole, 1);
		pItem->setData(Qt::UserRole+1, (unsigned int)iFF);
		listAtoms->addItem(pItem);
	}

	listAtoms->addItem(create_header_item("Ions"));
	for(std::size_t iFF=0; iFF<lstff->GetNumIons(); ++iFF)
	{
		const Formfact<t_real>& ff = lstff->GetIon(iFF);
		const std::string& strAtom = ff.GetAtomIdent();

		std::ostringstream ostrAtom;
		ostrAtom << strAtom;
		QListWidgetItem* pItem = new QListWidgetItem(ostrAtom.str().c_str());
		pItem->setData(Qt::UserRole, 2);
		pItem->setData(Qt::UserRole+1, (unsigned int)iFF);
		listAtoms->addItem(pItem);
	}
}

void FormfactorDlg::SetupMagAtoms()
{
	std::shared_ptr<const MagFormfactList<t_real>> lstff = MagFormfactList<t_real>::GetInstance();
	listMAtoms->addItem(create_header_item("Atoms / Ions"));

	bool bHad3d=0, bHad4d=0, bHad4f=0, bHad5f=0;
	for(std::size_t iFF=0; iFF<lstff->GetNumAtoms(); ++iFF)
	{
		const MagFormfact<t_real>& ff = lstff->GetAtom(iFF);
		const std::string& strAtom = ff.GetAtomIdent();

		if(strAtom.find("Sc") != std::string::npos && !bHad3d)
		{
			listMAtoms->addItem(create_header_item("3d (4s) Orbitals", 1));
			bHad3d = 1;
		}
		if(strAtom.find("Y") != std::string::npos && !bHad4d)
		{
			listMAtoms->addItem(create_header_item("4d (5s) Orbitals", 1));
			bHad4d = 1;
		}
		if(strAtom.find("Ce") != std::string::npos && !bHad4f)
		{
			listMAtoms->addItem(create_header_item("4f (5d 6s) Orbitals", 1));
			bHad4f = 1;
		}
		if((strAtom.find("Pa")!=std::string::npos || strAtom.find("U")!=std::string::npos)
			&& !bHad5f)
		{
			listMAtoms->addItem(create_header_item("5f (6d 7s) Orbitals", 1));
			bHad5f = 1;
		}

		// is the current one a d orbital?
		bool b_d_Orbital =(bHad4f==0 && bHad5f==0);

		std::ostringstream ostrAtom;
		ostrAtom << (iFF+1) << " " << strAtom;
		QListWidgetItem* pItem = new QListWidgetItem(ostrAtom.str().c_str());
		pItem->setData(Qt::UserRole, 1);
		pItem->setData(Qt::UserRole+1, (unsigned int)iFF);
		pItem->setData(Qt::UserRole+2, b_d_Orbital);
		listMAtoms->addItem(pItem);
	}
}

// ----------------------------------------------------------------------------


t_real FormfactorDlg::GetFormFact(t_real dQ) const
{
	if(!m_pCurAtom) return t_real(-1);
	const unsigned int iAtomOrIon = m_pCurAtom->data(Qt::UserRole).toUInt();
	if(iAtomOrIon == 0) return t_real(-1);
	const unsigned int iAtom = m_pCurAtom->data(Qt::UserRole+1).toUInt();

	std::shared_ptr<const FormfactList<t_real>> lstff = FormfactList<t_real>::GetInstance();
	if((iAtomOrIon==1 && iAtom < lstff->GetNumAtoms()) ||
		(iAtomOrIon==2 && iAtom < lstff->GetNumIons()))
	{
		const Formfact<t_real>& ff = (iAtomOrIon==1 ? lstff->GetAtom(iAtom) : lstff->GetIon(iAtom));
		const t_real dFF = ff.GetFormfact(dQ);
		return dFF;
	}

	return t_real(-1);
}


void FormfactorDlg::AtomSelected(QListWidgetItem *pItem, QListWidgetItem*)
{
	m_pCurAtom = pItem;
	if(!m_pCurAtom || !m_pCurAtom->data(Qt::UserRole).toUInt()) return;

	t_real dMinQ = 0.;
	t_real dMaxQ = 25.;

	m_vecQ.clear();
	m_vecFF.clear();

	m_vecQ.reserve(GFX_NUM_POINTS);
	m_vecFF.reserve(GFX_NUM_POINTS);

	for(std::size_t iPt=0; iPt<GFX_NUM_POINTS; ++iPt)
	{
		const t_real dQ = (dMinQ + (dMaxQ - dMinQ)/t_real(GFX_NUM_POINTS)*t_real(iPt));
		const t_real dFF = GetFormFact(dQ);

		m_vecQ.push_back(dQ);
		m_vecFF.push_back(dFF);
	}

	set_qwt_data<t_real>()(*m_plotwrap, m_vecQ, m_vecFF);
}


// ----------------------------------------------------------------------------


t_real FormfactorDlg::GetMagFormFact(t_real dQ) const
{
	if(!m_pCurMagAtom || !m_pCurMagAtom->data(Qt::UserRole).toUInt())
		return t_real(-1);

	t_real dL = spinL->value();
	t_real dS = spinS->value();
	t_real dJ = spinJ->value();
	t_real dG = sping->value();

	const unsigned int iAtom = m_pCurMagAtom->data(Qt::UserRole+1).toUInt();
	const bool b_d_Orbital = m_pCurMagAtom->data(Qt::UserRole+2).toBool();

	std::shared_ptr<const MagFormfactList<t_real>> lstff = MagFormfactList<t_real>::GetInstance();
	if(iAtom >= lstff->GetNumAtoms())
		return t_real(-1);

	const MagFormfact<t_real>& ff = lstff->GetAtom(iAtom);

	t_real dFF;
	if(b_d_Orbital)
		dFF = ff.GetFormfact(dQ, dG);
	else
		dFF = ff.GetFormfact(dQ, dL, dS, dJ);
	return dFF;
}


void FormfactorDlg::MagAtomSelected(QListWidgetItem *pItem, QListWidgetItem*)
{
	m_pCurMagAtom = pItem;
	if(!m_pCurMagAtom || !m_pCurMagAtom->data(Qt::UserRole).toUInt()) return;

	t_real dMinQ = 0.;
	t_real dMaxQ = 15.;

	m_vecQ_m.clear();
	m_vecFF_m.clear();

	m_vecQ_m.reserve(GFX_NUM_POINTS);
	m_vecFF_m.reserve(GFX_NUM_POINTS);


	for(std::size_t iPt=0; iPt<GFX_NUM_POINTS; ++iPt)
	{
		const t_real dQ = (dMinQ + (dMaxQ - dMinQ)/t_real(GFX_NUM_POINTS)*t_real(iPt));
		const t_real dFF = GetMagFormFact(dQ);

		m_vecQ_m.push_back(dQ);
		m_vecFF_m.push_back(dFF);
	}

	set_qwt_data<t_real>()(*m_plotwrap_m, m_vecQ_m, m_vecFF_m);
}


void FormfactorDlg::RefreshMagAtom()
{
	MagAtomSelected(listMAtoms->currentItem(), nullptr);
}

// ----------------------------------------------------------------------------

void FormfactorDlg::Calcg()
{
	t_real dL = spinL->value();
	t_real dS = spinS->value();
	t_real dJ = spinJ->value();

	t_real dg = tl::eff_gJ(dS, dL, dJ);
	sping->setValue(dg);


	std::ostringstream ostrMu;
	ostrMu.precision(g_iPrec);

	t_real muJ = tl::eff_magnetons(dg, dJ);
	t_real muS = tl::eff_magnetons(t_real(2), dS);	// spin only

	ostrMu << "mu_eff (spin-only) = " << muS << " mu_B, ";
	ostrMu << "mu_eff (total) = " << muJ << " mu_B.";
	labelStatus->setText(ostrMu.str().c_str());
}

void FormfactorDlg::CalcTermSymbol(const QString& qstr)
{
	try
	{
		std::string strOrbitals = qstr.toStdString();

		t_real dS, dL, dJ;
		std::tie(dS, dL, dJ) = tl::hund(strOrbitals);

		spinS->setValue(dS);
		spinL->setValue(dL);
		spinJ->setValue(dJ);
	}
	catch(const std::exception& ex)
	{
		tl::log_err(ex.what());
	}
}

// ----------------------------------------------------------------------------

void FormfactorDlg::SearchAtom(const QString& qstr)
{
	QList<QListWidgetItem*> lstItems = listAtoms->findItems(qstr, Qt::MatchContains);
	if(lstItems.size())
		listAtoms->setCurrentItem(lstItems[0], QItemSelectionModel::SelectCurrent);
}

void FormfactorDlg::SearchMagAtom(const QString& qstr)
{
	QList<QListWidgetItem*> lstItems = listMAtoms->findItems(qstr, Qt::MatchContains);
	if(lstItems.size())
		listMAtoms->setCurrentItem(lstItems[0], QItemSelectionModel::SelectCurrent);
}

void FormfactorDlg::SearchSLAtom(const QString& qstr)
{
	QList<QTableWidgetItem*> lstItems = tableSL->findItems(qstr, Qt::MatchContains);
	if(lstItems.size())
		tableSL->setCurrentItem(lstItems[0]);
}


void FormfactorDlg::PlotScatteringLengths()
{
	const bool bCoh = radioCoherent->isChecked();

	m_vecElem.clear();
	m_vecSc.clear();

	std::shared_ptr<const ScatlenList<t_real>> lstsc = ScatlenList<t_real>::GetInstance();
	for(std::size_t iAtom=0; iAtom<lstsc->GetNumElems(); ++iAtom)
	{
		const ScatlenList<t_real>::elem_type& sc = lstsc->GetElem(iAtom);
		std::complex<t_real> b = bCoh ? sc.GetCoherent() : sc.GetIncoherent();

		m_vecElem.push_back(t_real(iAtom+1));
		m_vecSc.push_back(b.real());
	}

	set_qwt_data<t_real>()(*m_plotwrapSc, m_vecElem, m_vecSc);
}


enum
{
	SL_ITEM_NR = 0,
	SL_ITEM_NAME = 1,
	SL_ITEM_COH_R = 2,
	SL_ITEM_COH_I = 3,
	SL_ITEM_INC_R = 4,
	SL_ITEM_INC_I = 5,
	SL_ITEM_ABUND = 6,
};

void FormfactorDlg::SetupScatteringLengths()
{
	std::shared_ptr<const ScatlenList<t_real>> lstsc = ScatlenList<t_real>::GetInstance();

	const bool bSortTable = tableSL->isSortingEnabled();
	tableSL->setSortingEnabled(0);

	tableSL->setRowCount(lstsc->GetNumElems() + lstsc->GetNumIsotopes());
	tableSL->setColumnWidth(SL_ITEM_NR, 50);
	tableSL->setColumnWidth(SL_ITEM_NAME, 70);
	tableSL->setColumnWidth(SL_ITEM_COH_R, 75);
	tableSL->setColumnWidth(SL_ITEM_COH_I, 75);
	tableSL->setColumnWidth(SL_ITEM_INC_R, 75);
	tableSL->setColumnWidth(SL_ITEM_INC_I, 75);
	tableSL->setColumnWidth(SL_ITEM_ABUND, 75);

	tableSL->verticalHeader()->setDefaultSectionSize(tableSL->verticalHeader()->minimumSectionSize()+2);

	for(std::size_t iElem=0; iElem<lstsc->GetNumElems()+lstsc->GetNumIsotopes(); ++iElem)
	{
		const typename ScatlenList<t_real>::elem_type* pelem = nullptr;
		if(iElem < lstsc->GetNumElems())
			pelem = &lstsc->GetElem(iElem);
		else
			pelem = &lstsc->GetIsotope(iElem-lstsc->GetNumElems());

		t_real dCohR = pelem->GetCoherent().real();
		t_real dCohI = pelem->GetCoherent().imag();
		t_real dIncR = pelem->GetIncoherent().real();
		t_real dIncI = pelem->GetIncoherent().imag();
		boost::optional<t_real> dAbund = pelem->GetAbundance();
		boost::optional<t_real> dHL = pelem->GetHalflife();
		const auto& vecIsotopes = pelem->GetIsotopes();

		std::string strCohR = tl::var_to_str(dCohR, g_iPrec) + " fm";
		std::string strCohI = tl::var_to_str(dCohI, g_iPrec) + " fm";
		std::string strIncR = tl::var_to_str(dIncR, g_iPrec) + " fm";
		std::string strIncI = tl::var_to_str(dIncI, g_iPrec) + " fm";

		std::string strAbund;
		if(vecIsotopes.size())	// if it is a mixture, list the isotopes
		{
			for(std::size_t iIsotope=0; iIsotope<vecIsotopes.size(); ++iIsotope)
			{
				strAbund += vecIsotopes[iIsotope]->GetAtomIdent();
				if(iIsotope+1 < vecIsotopes.size())
					strAbund += ", ";
			}
		}
		else	// else list the abundance or halflife
		{
			if(dHL)
				strAbund = std::string("HL = ") + tl::var_to_str(*dHL, g_iPrec) + " a";
			else if(dAbund)
				strAbund = tl::var_to_str(*dAbund, g_iPrec);
		}

		tableSL->setItem(iElem, SL_ITEM_NR, new QTableWidgetItemWrapper<std::size_t>(iElem+1));
		tableSL->setItem(iElem, SL_ITEM_NAME, new QTableWidgetItem(pelem->GetAtomIdent().c_str()));
		tableSL->setItem(iElem, SL_ITEM_COH_R, new QTableWidgetItemWrapper<t_real>(dCohR, strCohR));
		tableSL->setItem(iElem, SL_ITEM_COH_I, new QTableWidgetItemWrapper<t_real>(dCohI, strCohI));
		tableSL->setItem(iElem, SL_ITEM_INC_R, new QTableWidgetItemWrapper<t_real>(dIncR, strIncR));
		tableSL->setItem(iElem, SL_ITEM_INC_I, new QTableWidgetItemWrapper<t_real>(dIncI, strIncI));
		tableSL->setItem(iElem, SL_ITEM_ABUND, new QTableWidgetItemWrapper<t_real>(dAbund?*dAbund:0, strAbund));
	}

	tableSL->setSortingEnabled(bSortTable);
	tableSL->sortByColumn(SL_ITEM_NR, Qt::AscendingOrder);
}


void FormfactorDlg::cursorMoved(const QPointF& pt)
{
	const int iCurTabIdx = tabWidget->currentIndex();

	if(iCurTabIdx == 1 || iCurTabIdx == 2)		// magnetic & atomic form factors
	{
		t_real dX = pt.x();
		std::wstring strX = std::to_wstring(dX);
		std::wstring strY;
		
		if(iCurTabIdx == 1)		// magnetic
			strY = std::to_wstring(GetMagFormFact(dX));
		if(iCurTabIdx == 2)		// atomic
			strY = std::to_wstring(GetFormFact(dX));

		const std::wstring strAA = tl::get_spec_char_utf16("AA") +
			tl::get_spec_char_utf16("sup-") +
			tl::get_spec_char_utf16("sup1");

		std::wostringstream ostr;
		ostr << L"Q = " << strX << L" " << strAA;
		ostr << L", f = " << strY;

		labelStatus->setText(QString::fromWCharArray(ostr.str().c_str()));
	}
	else if(iCurTabIdx == 0)		// scattering lengths
	{
		std::shared_ptr<const ScatlenList<t_real>> lst = ScatlenList<t_real>::GetInstance();

		int iElem = std::round(pt.x());
		if(iElem<=0 || iElem>=int(lst->GetNumElems()))
		{
			labelStatus->setText("");
			return;
		}

		const std::string& strName = lst->GetElem(unsigned(iElem-1)).GetAtomIdent();
		std::complex<t_real> b_coh = lst->GetElem(unsigned(iElem-1)).GetCoherent();
		std::complex<t_real> b_inc = lst->GetElem(unsigned(iElem-1)).GetIncoherent();

		std::ostringstream ostr;
		ostr << iElem << ", " << strName
			<< ": b_coh = " << b_coh << ", b_inc = " << b_inc;
		labelStatus->setText(ostr.str().c_str());
	}
}


void FormfactorDlg::closeEvent(QCloseEvent* pEvt)
{
	QDialog::closeEvent(pEvt);
}


void FormfactorDlg::accept()
{
	if(m_pSettings)
		m_pSettings->setValue("formfactors/geo", saveGeometry());

	QDialog::accept();
}



#include "FormfactorDlg.moc"
