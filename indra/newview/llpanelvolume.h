/** 
 * @file llpanelvolume.h
 * @brief Object editing (position, scale, etc.) in the tools floater
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLPANELVOLUME_H
#define LL_LLPANELVOLUME_H

#include "v3math.h"
#include "llpanel.h"
#include "llmemory.h"
#include "llvolume.h"

class LLSpinCtrl;
class LLCheckBoxCtrl;
class LLTextBox;
class LLUICtrl;
class LLButton;
class LLViewerObject;
class LLComboBox;
class LLPanelInventory;
class LLColorSwatchCtrl;

class LLPanelVolume : public LLPanel
{
public:
	LLPanelVolume(const std::string& name);
	virtual ~LLPanelVolume();

	virtual void	draw();
	virtual void 	clearCtrls();

	virtual BOOL	postBuild();

	void			refresh();

	void			sendIsLight();
	void			sendIsFlexible();

	static BOOL		precommitValidate(LLUICtrl* ctrl,void* userdata);
	
	static void 	onCommitIsLight(		LLUICtrl* ctrl, void* userdata);
	static void 	onCommitLight(			LLUICtrl* ctrl, void* userdata);
	static void 	onCommitIsFlexible(		LLUICtrl* ctrl, void* userdata);
	static void 	onCommitFlexible(		LLUICtrl* ctrl, void* userdata);
#if MESH_ENABLED
	static void     onCommitPhysicsParam(       LLUICtrl* ctrl, void* userdata);
#endif //MESH_ENABLED

	static void 	onCommitMaterial(		LLUICtrl* ctrl, void* userdata);

	static void 	onCommitPhysicsShapeType   (LLUICtrl* ctrl, void* userdata);
	static void 	onCommitPhysicsGravity     (LLUICtrl* ctrl, void* userdata);
	static void 	onCommitPhysicsFriction    (LLUICtrl* ctrl, void* userdata);
	static void 	onCommitPhysicsDensity     (LLUICtrl* ctrl, void* userdata);
	static void 	onCommitPhysicsRestitution (LLUICtrl* ctrl, void* userdata);

	static void		onLightCancelColor(LLUICtrl* ctrl, void* userdata);
	static void		onLightSelectColor(LLUICtrl* ctrl, void* userdata);

	static void		onLightCancelTexture(LLUICtrl* ctrl, void* userdata);
	static void		onLightSelectTexture(LLUICtrl* ctrl, void* userdata);


protected:
	void			getState();
#if MESH_ENABLED
	void			refreshCost();
#endif //MESH_ENABLED

protected:
#if MESH_ENABLED
	void            sendPhysicsShapeType(LLUICtrl* ctrl, void* userdata);
	void            sendPhysicsGravity(LLUICtrl* ctrl, void* userdata);
	void            sendPhysicsFriction(LLUICtrl* ctrl, void* userdata);
	void            sendPhysicsRestitution(LLUICtrl* ctrl, void* userdata);
	void            sendPhysicsDensity(LLUICtrl* ctrl, void* userdata);
#endif //MESH_ENABLED
/*
	LLTextBox*		mLabelSelectSingleMessage;
	// Light
	LLCheckBoxCtrl*	mCheckLight;
	LLCheckBoxCtrl*	mCheckFlexible1D;
	LLTextBox*		mLabelColor;
	LLColorSwatchCtrl* mLightColorSwatch;
	LLSpinCtrl*		mLightIntensity;
	LLSpinCtrl*		mLightRadius;
	LLSpinCtrl*		mLightFalloff;
	LLSpinCtrl*		mLightCutoff;
	// Flexibile
	LLSpinCtrl*		mSpinSections;
	LLSpinCtrl*		mSpinGravity;
	LLSpinCtrl*		mSpinTension;
	LLSpinCtrl*		mSpinFriction;
	LLSpinCtrl*		mSpinWind;
	LLSpinCtrl*		mSpinForce[3];
*/

	S32				mComboMaterialItemCount;

	LLTextBox*		mLabelMaterial;
	LLComboBox*		mComboMaterial;
	
	LLColor4		mLightSavedColor;
	LLUUID			mLightSavedTexture;
	LLPointer<LLViewerObject> mObject;
	LLPointer<LLViewerObject> mRootObject;
#if MESH_ENABLED
	LLTextBox*		mComboPhysicsShapeLabel;
	LLComboBox*     mComboPhysicsShapeType;
	LLSpinCtrl*     mSpinPhysicsGravity;
	LLSpinCtrl*     mSpinPhysicsFriction;
	LLSpinCtrl*     mSpinPhysicsDensity;
	LLSpinCtrl*     mSpinPhysicsRestitution;
#endif //MESH_ENABLED
};

#endif
