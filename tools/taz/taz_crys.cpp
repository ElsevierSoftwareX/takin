/**
 * TAS tool (crystal stuff)
 * @author tweber
 * @date 2014 - 2016
 * @license GPLv2
 */

#include "taz.h"
#include "tlibs/string/string.h"
#include "tlibs/string/spec_char.h"
#include <boost/algorithm/string.hpp>


using t_real = t_real_glob;

static inline QString dtoqstr(t_real dVal, unsigned int iPrec=8)
{
	std::string str = tl::var_to_str(dVal, iPrec);
	return QString(str.c_str());
}

std::ostream& operator<<(std::ostream& ostr, const tl::Lattice<t_real>& lat)
{
	ostr << "a = " << lat.GetA();
	ostr << ", b = " << lat.GetB();
	ostr << ", c = " << lat.GetC();
	ostr << ", alpha = " << lat.GetAlpha();
	ostr << ", beta = " << lat.GetBeta();
	ostr << ", gamma = " << lat.GetGamma();
	return ostr;
}

void TazDlg::emitSampleParams()
{
	t_real a = editA->text().toDouble();
	t_real b = editB->text().toDouble();
	t_real c = editC->text().toDouble();

	t_real alpha = tl::d2r(editAlpha->text().toDouble());
	t_real beta = tl::d2r(editBeta->text().toDouble());
	t_real gamma = tl::d2r(editGamma->text().toDouble());

	t_real dX0 = editScatX0->text().toDouble();
	t_real dX1 = editScatX1->text().toDouble();
	t_real dX2 = editScatX2->text().toDouble();

	t_real dY0 = editScatY0->text().toDouble();
	t_real dY1 = editScatY1->text().toDouble();
	t_real dY2 = editScatY2->text().toDouble();

	SampleParams sampleparams;
	sampleparams.dAngles[0] = alpha; sampleparams.dAngles[1] = beta; sampleparams.dAngles[2] = gamma;
	sampleparams.dLattice[0] = a; sampleparams.dLattice[1] = b; sampleparams.dLattice[2] = c;
	sampleparams.dPlane1[0] = dX0; sampleparams.dPlane1[1] = dX1; sampleparams.dPlane1[2] = dX2;
	sampleparams.dPlane2[0] = dY0; sampleparams.dPlane2[1] = dY1; sampleparams.dPlane2[2] = dY2;
	emit SampleParamsChanged(sampleparams);
}

void TazDlg::SetCrystalType()
{
	m_crystalsys = CrystalSystem::CRYS_NOT_SET;

	SpaceGroup<t_real> *pSpaceGroup = 0;
	int iSpaceGroupIdx = comboSpaceGroups->currentIndex();
	if(iSpaceGroupIdx != 0)
		pSpaceGroup = (SpaceGroup<t_real>*)comboSpaceGroups->itemData(iSpaceGroupIdx).value<void*>();
	if(pSpaceGroup)
		m_crystalsys = pSpaceGroup->GetCrystalSystem();

	CheckCrystalType();
}

void TazDlg::CheckCrystalType()
{
	set_crystal_system_edits(m_crystalsys, editA, editB, editC,
		editAlpha, editBeta, editGamma,
		editARecip, editBRecip, editCRecip,
		editAlphaRecip, editBetaRecip, editGammaRecip);
}

void TazDlg::CalcPeaksRecip()
{
	if(!m_bReady) return;

	try
	{
		t_real a = editARecip->text().toDouble();
		t_real b = editBRecip->text().toDouble();
		t_real c = editCRecip->text().toDouble();

		t_real alpha = tl::d2r(editAlphaRecip->text().toDouble());
		t_real beta = tl::d2r(editBetaRecip->text().toDouble());
		t_real gamma = tl::d2r(editGammaRecip->text().toDouble());

		tl::Lattice<t_real> lattice(a,b,c, alpha,beta,gamma);
		tl::Lattice<t_real> recip = lattice.GetRecip();

		editA->setText(dtoqstr(recip.GetA(), g_iPrec));
		editB->setText(dtoqstr(recip.GetB(), g_iPrec));
		editC->setText(dtoqstr(recip.GetC(), g_iPrec));
		editAlpha->setText(dtoqstr(tl::r2d(recip.GetAlpha()), g_iPrec));
		editBeta->setText(dtoqstr(tl::r2d(recip.GetBeta()), g_iPrec));
		editGamma->setText(dtoqstr(tl::r2d(recip.GetGamma()), g_iPrec));

		m_bUpdateRecipEdits = 0;
		CalcPeaks();
		m_bUpdateRecipEdits = 1;
	}
	catch(const std::exception& ex)
	{
		tl::log_err(ex.what());
	}
}

void TazDlg::CalcPeaks()
{
	if(!m_bReady || !m_sceneRecip.GetTriangle() || !m_sceneRealLattice.GetLattice())
		return;

	try
	{
		const bool bPowder = checkPowder->isChecked();

		// lattice
		const t_real a = editA->text().toDouble();
		const t_real b = editB->text().toDouble();
		const t_real c = editC->text().toDouble();

		const t_real alpha = tl::d2r(editAlpha->text().toDouble());
		const t_real beta = tl::d2r(editBeta->text().toDouble());
		const t_real gamma = tl::d2r(editGamma->text().toDouble());
#ifndef NDEBUG
		tl::log_debug("lattice consts = ", a, ", ", b, ", ", c);
		tl::log_debug("lattice angles = ", alpha, ", ", beta, ", ", gamma);
#endif

		tl::Lattice<t_real> lattice(a,b,c, alpha,beta,gamma);
		tl::Lattice<t_real> recip_unrot = lattice.GetRecip();


		//----------------------------------------------------------------------
		// scattering plane
		t_real dX0 = editScatX0->text().toDouble();
		t_real dX1 = editScatX1->text().toDouble();
		t_real dX2 = editScatX2->text().toDouble();
		ublas::vector<t_real> vecPlaneXRLU = tl::make_vec({dX0, dX1, dX2});

		t_real dY0 = editScatY0->text().toDouble();
		t_real dY1 = editScatY1->text().toDouble();
		t_real dY2 = editScatY2->text().toDouble();
		ublas::vector<t_real> vecPlaneYRLU = tl::make_vec({dY0, dY1, dY2});

		//----------------------------------------------------------------------
		// show integer up vector
		unsigned int iMaxDec = 4;	// TODO: determine max. # of entered decimals
		ublas::vector<int> ivecUp = tl::cross_3(
			tl::make_vec<ublas::vector<int>>({int(dX0*std::pow(10, iMaxDec)),
				int(dX1*std::pow(10, iMaxDec)),
				int(dX2*std::pow(10, iMaxDec))}),
			tl::make_vec<ublas::vector<int>>({int(dY0*std::pow(10, iMaxDec)),
				int(dY1*std::pow(10, iMaxDec)),
				int(dY2*std::pow(10, iMaxDec))}));
		ivecUp = tl::get_gcd_vec(ivecUp);
		editScatZ0->setText(tl::var_to_str(ivecUp[0], g_iPrec).c_str());
		editScatZ1->setText(tl::var_to_str(ivecUp[1], g_iPrec).c_str());
		editScatZ2->setText(tl::var_to_str(ivecUp[2], g_iPrec).c_str());
		//----------------------------------------------------------------------

		ublas::vector<t_real> vecX0 = ublas::zero_vector<t_real>(3);
		tl::Plane<t_real> planeRLU(vecX0, vecPlaneXRLU, vecPlaneYRLU);
		if(!planeRLU.IsValid())
		{
			tl::log_err("Invalid scattering plane.");
			return;
		}
		//----------------------------------------------------------------------


		//----------------------------------------------------------------------
		// view plane for real lattice
		// scattering plane
		t_real dX0R = editRealX0->text().toDouble();
		t_real dX1R = editRealX1->text().toDouble();
		t_real dX2R = editRealX2->text().toDouble();
		ublas::vector<t_real> vecPlaneXR = tl::make_vec({dX0R, dX1R, dX2R});

		t_real dY0R = editRealY0->text().toDouble();
		t_real dY1R = editRealY1->text().toDouble();
		t_real dY2R = editRealY2->text().toDouble();
		ublas::vector<t_real> vecPlaneYR = tl::make_vec({dY0R, dY1R, dY2R});

		//----------------------------------------------------------------------
		// show integer up vector
		unsigned int iMaxDecR = 4;	// TODO: determine max. # of entered decimals
		ublas::vector<int> ivecUpR = tl::cross_3(
			tl::make_vec<ublas::vector<int>>({int(dX0R*std::pow(10, iMaxDecR)),
				int(dX1R*std::pow(10, iMaxDecR)),
				int(dX2R*std::pow(10, iMaxDecR))}),
			tl::make_vec<ublas::vector<int>>({int(dY0R*std::pow(10, iMaxDecR)),
				int(dY1R*std::pow(10, iMaxDecR)),
				int(dY2R*std::pow(10, iMaxDecR))}));
		ivecUpR = tl::get_gcd_vec(ivecUpR);
		editRealZ0->setText(tl::var_to_str(ivecUpR[0], g_iPrec).c_str());
		editRealZ1->setText(tl::var_to_str(ivecUpR[1], g_iPrec).c_str());
		editRealZ2->setText(tl::var_to_str(ivecUpR[2], g_iPrec).c_str());
		//----------------------------------------------------------------------

		ublas::vector<t_real> vecX0R = ublas::zero_vector<t_real>(3);
		tl::Plane<t_real> planeRealFrac(vecX0R, vecPlaneXR, vecPlaneYR);
		if(!planeRealFrac.IsValid())
		{
			tl::log_err("Invalid view plane for real lattice.");
			return;
		}
		//----------------------------------------------------------------------


		/*
		// rotated lattice
		t_real dPhi = spinRotPhi->value() / 180. * M_PI;
		t_real dTheta = spinRotTheta->value() / 180. * M_PI;
		t_real dPsi = spinRotPsi->value() / 180. * M_PI;
		//lattice.RotateEuler(dPhi, dTheta, dPsi);

		ublas::vector<t_real> dir0 = plane.GetDir0();
		ublas::vector<t_real> dirup = plane.GetNorm();
		ublas::vector<t_real> dir1 = tl::cross_3(dirup, dir0);

		t_real dDir0Len = ublas::norm_2(dir0);
		t_real dDir1Len = ublas::norm_2(dir1);
		t_real dDirUpLen = ublas::norm_2(dirup);

		if(tl::float_equal(dDir0Len, 0.) || tl::float_equal(dDir1Len, 0.) || tl::float_equal(dDirUpLen, 0.)
			|| tl::is_nan_or_inf<t_real>(dDir0Len) || tl::is_nan_or_inf<t_real>(dDir1Len) || tl::is_nan_or_inf<t_real>(dDirUpLen))
		{
			tl::log_err("Invalid scattering plane.");
			return;
		}

		dir0 /= dDir0Len;
		dir1 /= dDir1Len;
		//dirup /= dDirUpLen;
		*/

		if(m_pGotoDlg)
		{
			m_pGotoDlg->SetLattice(lattice);
			m_pGotoDlg->SetScatteringPlane(tl::make_vec({dX0, dX1, dX2}), tl::make_vec({dY0, dY1, dY2}));
			m_pGotoDlg->CalcSample();
		}

		emitSampleParams();

		//lattice.RotateEulerRecip(dir0, dir1, dirup, dPhi, dTheta, dPsi);
		//tl::Lattice<t_real> recip = lattice.GetRecip();
		const tl::Lattice<t_real>& recip = recip_unrot;		// anyway not rotated anymore


		if(m_bUpdateRecipEdits)
		{
			editARecip->setText(dtoqstr(recip.GetA(), g_iPrec));
			editBRecip->setText(dtoqstr(recip.GetB(), g_iPrec));
			editCRecip->setText(dtoqstr(recip.GetC(), g_iPrec));
			editAlphaRecip->setText(dtoqstr(tl::r2d(recip.GetAlpha()), g_iPrec));
			editBetaRecip->setText(dtoqstr(tl::r2d(recip.GetBeta()), g_iPrec));
			editGammaRecip->setText(dtoqstr(tl::r2d(recip.GetGamma()), g_iPrec));
		}

		const std::wstring& strAA = tl::get_spec_char_utf16("AA");
		const std::wstring& strMinus = tl::get_spec_char_utf16("sup-");
		const std::wstring& strThree = tl::get_spec_char_utf16("sup3");

		t_real dVol = lattice.GetVol();
		t_real dVol_recip = recip.GetVol() /*/ (2.*M_PI*2.*M_PI*2.*M_PI)*/;

		std::wostringstream ostrSample;
		ostrSample.precision(g_iPrecGfx);
		ostrSample << "Unit Cell Volume: ";
		ostrSample << "Real: " << dVol << " " << strAA << strThree;
		ostrSample << ", Recip.: " << dVol_recip << " " << strAA << strMinus << strThree;
		//tl::log_info(tl::wstr_to_str(ostrSample.str()));
		m_pStatusMsg->setText(QString::fromWCharArray(ostrSample.str().c_str()));


		std::string strCryTy = "<not set>";
		SpaceGroup<t_real>* pSpaceGroup = nullptr;
		int iSpaceGroupIdx = comboSpaceGroups->currentIndex();
		if(iSpaceGroupIdx != 0)
			pSpaceGroup = (SpaceGroup<t_real>*)comboSpaceGroups->itemData(iSpaceGroupIdx).value<void*>();

		if(pSpaceGroup)
			strCryTy = pSpaceGroup->GetCrystalSystemName();

		editCrystalSystem->setText(strCryTy.c_str());


		m_latticecommon = LatticeCommon<t_real_glob>();
		if(m_latticecommon.Calc(lattice, recip, planeRLU, planeRealFrac, pSpaceGroup, &m_vecAtoms))
		{
			m_sceneRecip.GetTriangle()->CalcPeaks(m_latticecommon, bPowder);
			if(m_sceneRecip.getSnapq())
				m_sceneRecip.GetTriangle()->SnapToNearestPeak(m_sceneRecip.GetTriangle()->GetNodeGq());
			m_sceneRecip.emitUpdate();

			m_sceneProjRecip.GetLattice()->CalcPeaks(m_latticecommon, true);

			m_sceneRealLattice.GetLattice()->CalcPeaks(m_latticecommon);
#ifndef NO_3D
			if(m_pRecip3d)
				m_pRecip3d->CalcPeaks(m_latticecommon);
#endif
		}
		else
		{
			tl::log_err("Lattice calculations failed.");
		}

		m_dlgRealParam.CrystalChanged(lattice, recip, pSpaceGroup, &m_vecAtoms);
	}
	catch(const std::exception& ex)
	{
		m_sceneRecip.GetTriangle()->ClearPeaks();
		tl::log_err(ex.what());
	}
}

void TazDlg::RotatePlane(unsigned iAxis, t_real dAngle)
{
	m_bReady = false;

	t_real dX0 = editScatX0->text().toDouble();
	t_real dX1 = editScatX1->text().toDouble();
	t_real dX2 = editScatX2->text().toDouble();
	ublas::vector<t_real> vecX = tl::make_vec({dX0, dX1, dX2});

	t_real dY0 = editScatY0->text().toDouble();
	t_real dY1 = editScatY1->text().toDouble();
	t_real dY2 = editScatY2->text().toDouble();
	ublas::vector<t_real> vecY = tl::make_vec({dY0, dY1, dY2});

	ublas::vector<t_real> vecZ = tl::cross_3(vecX, vecY);

	std::vector<ublas::vector<t_real>> vecOrth =
		tl::gram_schmidt<ublas::vector<t_real>>
			({vecX, vecY, vecZ}, 1);

	ublas::matrix<t_real> matRot =
		tl::rotation_matrix(vecOrth[iAxis], dAngle);
	vecX = ublas::prod(matRot, vecOrth[0]);
	vecY = ublas::prod(matRot, vecOrth[1]);

	tl::set_eps_0(vecX, g_dEps);
	tl::set_eps_0(vecY, g_dEps);

	editScatX0->setText(tl::var_to_str(vecX[0], g_iPrec).c_str());
	editScatX1->setText(tl::var_to_str(vecX[1], g_iPrec).c_str());
	editScatX2->setText(tl::var_to_str(vecX[2], g_iPrec).c_str());
	editScatY0->setText(tl::var_to_str(vecY[0], g_iPrec).c_str());
	editScatY1->setText(tl::var_to_str(vecY[1], g_iPrec).c_str());
	editScatY2->setText(tl::var_to_str(vecY[2], g_iPrec).c_str());

	m_bReady = true;
	CalcPeaks();
}

void TazDlg::RepopulateSpaceGroups()
{
	std::shared_ptr<const SpaceGroups<t_real>> sgs = SpaceGroups<t_real>::GetInstance();
	const SpaceGroups<t_real>::t_mapSpaceGroups* pmapSpaceGroups = sgs->get_space_groups();
	if(!pmapSpaceGroups)
		return;

	QString strCurSG = comboSpaceGroups->currentText();
	comboSpaceGroups->setCurrentIndex(0);

	for(int iCnt=comboSpaceGroups->count()-1; iCnt>0; --iCnt)
		comboSpaceGroups->removeItem(iCnt);

	std::string strFilter = editSpaceGroupsFilter->text().toStdString();

	for(const SpaceGroups<t_real>::t_mapSpaceGroups::value_type& pair : *pmapSpaceGroups)
	{
		const std::string& strName = pair.second.GetName();

		typedef const boost::iterator_range<std::string::const_iterator> t_striterrange;
		if(strFilter!="" &&
			!boost::ifind_first(t_striterrange(strName.begin(), strName.end()),
			t_striterrange(strFilter.begin(), strFilter.end())))
			continue;

		comboSpaceGroups->insertItem(comboSpaceGroups->count(),
			strName.c_str(), QVariant::fromValue((void*)&pair.second));
	}


	int iSGIdx = comboSpaceGroups->findText(strCurSG);
	if(iSGIdx >= 0)
		comboSpaceGroups->setCurrentIndex(iSGIdx);
}


//--------------------------------------------------------------------------------
// spurion stuff

void TazDlg::ShowSpurions()
{
	if(!m_pSpuri)
	{
		m_pSpuri = new SpurionDlg(this, &m_settings);

		QObject::connect(&m_sceneRecip, SIGNAL(paramsChanged(const RecipParams&)),
			m_pSpuri, SLOT(paramsChanged(const RecipParams&)));

		m_sceneRecip.emitAllParams();
	}

	m_pSpuri->show();
	m_pSpuri->activateWindow();
}

void TazDlg::spurionInfo(const tl::ElasticSpurion& spuri,
	const std::vector<tl::InelasticSpurion<t_real>>& vecInelCKI,
	const std::vector<tl::InelasticSpurion<t_real>>& vecInelCKF)
{
	std::ostringstream ostrMsg;
	if(spuri.bAType || spuri.bMType || vecInelCKI.size() || vecInelCKF.size())
		ostrMsg << "Warning: ";

	if(spuri.bAType || spuri.bMType)
	{
		ostrMsg << "Spurious elastic scattering of type ";
		if(spuri.bAType && spuri.bMType)
		{
			ostrMsg << "A and M";
			ostrMsg << (spuri.bAKfSmallerKi ? " (kf<ki)" : " (kf>ki)");
		}
		else if(spuri.bAType)
		{
			ostrMsg << "A";
			ostrMsg << (spuri.bAKfSmallerKi ? " (kf<ki)" : " (kf>ki)");
		}
		else if(spuri.bMType)
		{
			ostrMsg << "M";
			ostrMsg << (spuri.bMKfSmallerKi ? " (kf<ki)" : " (kf>ki)");
		}
		ostrMsg << ".";

		if(vecInelCKI.size() || vecInelCKF.size())
			ostrMsg << " ";
	}

	const std::string& strDelta = tl::get_spec_char_utf8("Delta");

	if(vecInelCKI.size())
	{
		ostrMsg << "Spurious inelastic CKI scattering at "
				<< strDelta << "E = ";
		for(unsigned int iInel=0; iInel<vecInelCKI.size(); ++iInel)
		{
			ostrMsg << vecInelCKI[iInel].dE_meV << " meV";
			if(iInel != vecInelCKI.size()-1)
				ostrMsg << ", ";
		}
		ostrMsg << ".";

		if(vecInelCKF.size())
			ostrMsg << " ";
	}

	if(vecInelCKF.size())
	{
		ostrMsg << "Spurious inelastic CKF scattering at "
				<< strDelta << "E = ";
		for(unsigned int iInel=0; iInel<vecInelCKF.size(); ++iInel)
		{
			ostrMsg << vecInelCKF[iInel].dE_meV << " meV";
			if(iInel != vecInelCKF.size()-1)
				ostrMsg << ", ";
		}
		ostrMsg << ".";
	}

	m_pStatusMsg->setText(QString::fromUtf8(ostrMsg.str().c_str(), ostrMsg.str().size()));
}



//--------------------------------------------------------------------------------
// reso stuff
void TazDlg::InitReso()
{
	if(!m_pReso)
	{
		m_pReso = new ResoDlg(this, &m_settings);

		QObject::connect(this, SIGNAL(ResoParamsChanged(const ResoParams&)),
			m_pReso, SLOT(ResoParamsChanged(const ResoParams&)));
		QObject::connect(&m_sceneRecip, SIGNAL(paramsChanged(const RecipParams&)),
			m_pReso, SLOT(RecipParamsChanged(const RecipParams&)));
		QObject::connect(&m_sceneReal, SIGNAL(paramsChanged(const RealParams&)),
			m_pReso, SLOT(RealParamsChanged(const RealParams&)));
		QObject::connect(this, SIGNAL(SampleParamsChanged(const SampleParams&)),
			m_pReso, SLOT(SampleParamsChanged(const SampleParams&)));

		UpdateDs();
		UpdateMonoSense();
		UpdateSampleSense();
		UpdateAnaSense();

		emitSampleParams();
		m_sceneRecip.emitAllParams();
		m_sceneReal.emitAllParams();
	}
}

void TazDlg::ShowResoParams()
{
	InitReso();

	m_pReso->show();
	m_pReso->activateWindow();
}

void TazDlg::ShowResoEllipses()
{
	InitReso();

	if(!m_pEllipseDlg)
	{
		m_pEllipseDlg = new EllipseDlg(this, &m_settings);
		QObject::connect(m_pReso, SIGNAL(ResoResultsSig(const EllipseDlgParams&)),
			m_pEllipseDlg, SLOT(SetParams(const EllipseDlgParams&)));

		m_pReso->EmitResults();
	}

	m_pEllipseDlg->show();
	m_pEllipseDlg->activateWindow();
}

void TazDlg::ShowResoConv()
{
	if(!m_pConvoDlg)
		m_pConvoDlg = new ConvoDlg(this, &m_settings);

	m_pConvoDlg->show();
	m_pConvoDlg->activateWindow();
}

#ifndef NO_3D
void TazDlg::ShowResoEllipses3D()
{
	InitReso();

	if(!m_pEllipseDlg3D)
	{
		m_pEllipseDlg3D = new EllipseDlg3D(this, &m_settings);
		QObject::connect(m_pReso,
			SIGNAL(ResoResultsSig(const EllipseDlgParams&)),
			m_pEllipseDlg3D, SLOT(SetParams(const EllipseDlgParams&)));

		m_pReso->EmitResults();
	}

	m_pEllipseDlg3D->show();
	m_pEllipseDlg3D->activateWindow();
}

#else
void TazDlg::ShowResoEllipses3D() {}
#endif



//--------------------------------------------------------------------------------
// spacegroups & formfactors dialogs

void TazDlg::ShowFormfactorDlg()
{
	if(!m_pFormfactorDlg)
		m_pFormfactorDlg = new FormfactorDlg(this, &m_settings);
	m_pFormfactorDlg->show();
	m_pFormfactorDlg->activateWindow();
}

void TazDlg::ShowAtomsDlg()
{
	if(!m_pAtomsDlg)
	{
		m_pAtomsDlg = new AtomsDlg(this, &m_settings);
		QObject::connect(m_pAtomsDlg, SIGNAL(ApplyAtoms(const std::vector<AtomPos<t_real_glob>>&)),
			this, SLOT(ApplyAtoms(const std::vector<AtomPos<t_real_glob>>&)));
	}

	m_pAtomsDlg->SetAtoms(m_vecAtoms);
	m_pAtomsDlg->show();
	m_pAtomsDlg->activateWindow();
}

void TazDlg::ApplyAtoms(const std::vector<AtomPos<t_real>>& vecAtoms)
{
	m_vecAtoms = vecAtoms;
	CalcPeaks();
}

void TazDlg::ShowSgListDlg()
{
	if(!m_pSgListDlg)
		m_pSgListDlg = new SgListDlg(this);
	m_pSgListDlg->show();
	m_pSgListDlg->activateWindow();
}