/*

  Nullsoft WASABI Source File License

  Copyright 1999-2003 Nullsoft, Inc.

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.

*/
#include "precomp.h"
#include "ds2.h"
#include <math.h>

#ifdef DS2_HAVE_FADES


static double lin2log_vol(double v)
{
	return v>0 ? 20.0*log10(v) : -100.0;
}

static double log2lin_vol(double v)
{
	return v<=-100.0 ? 0 : pow(10,v/20.0);
}

static double lin2log_pan(double p)
{
	if (p==0) return 0;
	else return lin2log_vol(1.0-fabs(p)) * (p>0 ? -1 : 1);
}

static double log2lin_pan(double p)
{
	if (p==0) return 0;
	else return (1.0-log2lin_vol(-fabs(p))) * (p>0 ? 1 :-1);
}

void DS2::DsVolCtrl::MapVol(double Vol,double Pan,double &OutNewVol,double &OutNewPan)
{
	DestVolHack=Vol;
	DestPanHack=Pan;
	
	double NewVol,NewPan;

	switch(VolMode)
	{
	case 0:
		NewVol=lin2log_vol(Vol);
		NewPan=lin2log_pan(Pan);

		NewVol=Vol>0 ? 20*log10(Vol) : -100.0;//in negative db

		if (Pan==0) NewPan=0;
		else
		{
			double d= 1.0 - fabs(Pan);
			d = d>0 ? 20*log10(d) : -1000.0;
			if (Pan>0) d=-d;
			NewPan=d;
		}
		break;
	case 1:
		{
			double left,right;
			left=right=(double)LogVolMin * (Vol-1.0);
		

			left+=lin2log_vol(sqrt(0.5-0.5*Pan));
			right+=lin2log_vol(sqrt(0.5+0.5*Pan));
			
			NewVol=left>right ? left : right;
			NewPan=right-left;

		}
		break;
	case 2:
		{
			double left,right;
			left=right=100.0 * (pow(Vol,0.25)-1.0);
		
			left+=lin2log_vol(sqrt(0.5-0.5*Pan));
			right+=lin2log_vol(sqrt(0.5+0.5*Pan));
			
			NewVol=left>right ? left : right;
			NewPan=right-left;

		}
		break;
	}

	
	if (NewVol<-100.0) NewVol=-100.0;
	else if (NewVol>0) NewVol=0;
	if (NewPan<-100.0) NewPan=-100.0;
	else if (NewPan>100.0) NewPan=100.0;

	OutNewVol=NewVol;
	OutNewPan=NewPan;
}
	
DS2::DsVolCtrl::DsVolCtrl(int _VolMode,double _LogVolMin,bool _LogFades)
{
	IsFading=0;
	LogFades=_LogFades;
	VolMode=_VolMode;
	LogVolMin=_LogVolMin;
	FadeSrcTime=FadeDstTime=-1;
	CurTime=0;
	CurVol=1;
	LastVol=0;
	CurPan=0;
	LastPan=0;
}

void DS2::DsVolCtrl::SetFade(__int64 duration,double destvol,double destpan)
{
	FadeSrcTime=CurTime;
	FadeDstTime=CurTime+duration;
	FadeSrcVol=CurVol;
	FadeSrcPan=CurPan;
	IsFading=1;
	MapVol(destvol,destpan,FadeDstVol,FadeDstPan);
}

void DS2::DsVolCtrl::SetTime(__int64 time)
{
	CurTime=time;
}

void DS2::DsVolCtrl::SetVolume(double vol)
{
	if (Fading()) SetFade(FadeDstTime-CurTime,vol,DestPanHack);
	else MapVol(vol,DestPanHack,CurVol,CurPan);
}

void DS2::DsVolCtrl::SetPan(double pan)
{
	if (Fading()) SetFade(FadeDstTime-CurTime,DestVolHack,pan);
	else MapVol(DestVolHack,pan,CurVol,CurPan);
}

void DS2::DsVolCtrl::Apply(IDirectSoundBuffer * pDSB)
{
	if (Fading())
	{
		if (LogFades)
		{
			CurVol= FadeSrcVol + (FadeDstVol-FadeSrcVol) * (double)(CurTime-FadeSrcTime) / (double)(FadeDstTime-FadeSrcTime);
			CurPan= FadeSrcPan + (FadeDstPan-FadeSrcPan) * (double)(CurTime-FadeSrcTime) / (double)(FadeDstTime-FadeSrcTime);
		}
		else
		{
			double SrcVol=log2lin_vol(FadeSrcVol);
			double DstVol=log2lin_vol(FadeDstVol);
			double SrcPan=log2lin_pan(FadeSrcPan);
			double DstPan=log2lin_pan(FadeDstPan);

			CurVol=lin2log_vol( SrcVol + (DstVol-SrcVol) * (double)(CurTime-FadeSrcTime) / (double)(FadeDstTime-FadeSrcTime) );
			CurPan=lin2log_pan( SrcPan + (DstPan-SrcPan) * (double)(CurTime-FadeSrcTime) / (double)(FadeDstTime-FadeSrcTime) );
		}
	}
	else if (FadeDstTime>=0)
	{
		CurVol=FadeDstVol;
		CurPan=FadeDstPan;
		FadeDstTime=-1;
		IsFading=0;
	}
	
	if (CurVol!=LastVol)
	{
		LastVol=CurVol;
		pDSB->SetVolume((long)(CurVol*100.0));
	}

	if (CurPan!=LastPan)
	{
		LastPan=CurPan;
		pDSB->SetPan((long)(CurPan*100.0));
	}

}

bool DS2::DsVolCtrl::Fading()
{
	return IsFading && CurTime<FadeDstTime;
}

__int64 DS2::DsVolCtrl::RelFade(__int64 max,double destvol)
{
	return (__int64)(fabs(destvol-DestVolHack)*(double)max);
}

double DS2::DsVolCtrl::Stat_GetVolLeft()
{
	return CurPan<0 ? CurVol : CurVol - CurPan;
}

double DS2::DsVolCtrl::Stat_GetVolRight()
{
	return CurPan>0 ? CurVol : CurVol + CurPan;
}


#else

void DS2::DsVolCtrl::Apply(IDirectSoundBuffer * pDSB)
{
	double _vol=CurVol>0 ? 20*log10(CurVol) : -100.0;//in negative db
	pDSB->SetVolume((long) (_vol * 100.0));

	if (CurPan==0) pDSB->SetPan(0);
	else
	{
		double d= 1.0 - fabs(CurPan);
		d = d>0 ? 20*log10(d) : -1000.0;
		if (CurPan>0) d=-d;
		pDSB->SetPan((long)(d * 100.0));
	}
}

#endif
