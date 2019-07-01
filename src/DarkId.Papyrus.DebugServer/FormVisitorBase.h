#pragma once

#include "f4se/GameForms.h"
#include "f4se/GameObjects.h"
#include "f4se/GameReferences.h"

#define FORM_TYPE_LIST(code) \
    code(BGSKeyword, BGSKeyword, KYWD) \
    code(BGSLocationRefType, BGSLocationRefType, LCRT) \
    code(BGSAction, BGSAction, AACT) \
    code(BGSTextureSet, BGSTextureSet, TXST) \
    code(BGSMenuIcon, BGSMenuIcon, MICN) \
    code(TESGlobal, TESGlobal, GLOB) \
    code(TESClass, TESClass, CLAS) \
    code(TESFaction, TESFaction, FACT) \
    code(BGSHeadPart, BGSHeadPart, HDPT) \
    code(TESEyes, TESEyes, EYES) \
    code(TESRace, TESRace, RACE) \
    code(TESSound, TESSound, SOUN) \
    code(BGSAcousticSpace, BGSAcousticSpace, ASPC) \
    code(EffectSetting, EffectSetting, MGEF) \
    code(Script, Script, SCPT) \
    code(TESLandTexture, TESLandTexture, LTEX) \
    code(EnchantmentItem, EnchantmentItem, ENCH) \
    code(SpellItem, SpellItem, SPEL) \
    code(ScrollItem, ScrollItem, SCRL) \
    code(TESObjectACTI, TESObjectACTI, ACTI) \
    code(BGSTalkingActivator, BGSTalkingActivator, TACT) \
    code(TESObjectARMO, TESObjectARMO, ARMO) \
    code(TESObjectBOOK, TESObjectBOOK, BOOK) \
    code(TESObjectCONT, TESObjectCONT, CONT) \
    code(TESObjectDOOR, TESObjectDOOR, DOOR) \
    code(IngredientItem, IngredientItem, INGR) \
    code(TESObjectLIGH, TESObjectLIGH, LIGH) \
    code(TESObjectMISC, TESObjectMISC, MISC) \
    code(TESObjectSTAT, TESObjectSTAT, STAT) \
    code(BGSStaticCollection, BGSStaticCollection, SCOL) \
    code(BGSMovableStatic, BGSMovableStatic, MSTT) \
    code(TESGrass, TESGrass, GRAS) \
    code(TESObjectTREE, TESObjectTREE, TREE) \
    code(TESFlora, TESFlora, FLOR) \
    code(TESFurniture, TESFurniture, FURN) \
    code(TESObjectWEAP, TESObjectWEAP, WEAP) \
    code(TESAmmo, TESAmmo, AMMO) \
    code(TESNPC, TESNPC, NPC_) \
    code(TESLevCharacter, TESLevCharacter, LVLN) \
    code(TESKey, TESKey, KEYM) \
    code(AlchemyItem, AlchemyItem, ALCH) \
    code(BGSIdleMarker, BGSIdleMarker, IDLM) \
    code(BGSNote, BGSNote, NOTE) \
    code(BGSProjectile, BGSProjectile, PROJ) \
    code(BGSHazard, BGSHazard, HAZD) \
    code(TESSoulGem, TESSoulGem, SLGM) \
    code(BGSTerminal, BGSTerminal, TERM) \
    code(TESLevItem, TESLevItem, LVLI) \
    code(TESWeather, TESWeather, WTHR) \
    code(TESClimate, TESClimate, CLMT) \
    code(BGSShaderParticleGeometryData, BGSShaderParticleGeometryData, SPGD) \
    code(BGSReferenceEffect, BGSReferenceEffect, RFCT) \
    code(TESRegion, TESRegion, REGN) \
    code(NavMeshInfoMap, NavMeshInfoMap, NAVI) \
    code(TESObjectCELL, TESObjectCELL, CELL) \
    code(TESObjectREFR, TESObjectREFR, REFR) \
    code(PlayerCharacter, PlayerCharacter, ACHR) \
    code(MissileProjectile, MissileProjectile, PMIS) \
    code(ArrowProjectile, ArrowProjectile, PARW) \
    code(GrenadeProjectile, GrenadeProjectile, PGRE) \
    code(BeamProjectile, BeamProjectile, PBEA) \
    code(FlameProjectile, FlameProjectile, PFLA) \
    code(ConeProjectile, ConeProjectile, PCON) \
    code(BarrierProjectile, BarrierProjectile, PBAR) \
    code(Hazard, Hazard, PHZD) \
    code(TESWorldSpace, TESWorldSpace, WRLD) \
    code(TESObjectLAND, TESObjectLAND, LAND) \
    code(NavMesh, NavMesh, NAVM) \
    code(TESTopic, TESTopic, DIAL) \
    code(TESTopicInfo, TESTopicInfo, INFO) \
    code(TESQuest, TESQuest, QUST) \
    code(TESIdleForm, TESIdleForm, IDLE) \
    code(TESPackage, TESPackage, PACK) \
    code(TESCombatStyle, TESCombatStyle, CSTY) \
    code(TESLoadScreen, TESLoadScreen, LSCR) \
    code(TESLevSpell, TESLevSpell, LVSP) \
    code(TESObjectANIO, TESObjectANIO, ANIO) \
    code(TESWaterForm, TESWaterForm, WATR) \
    code(TESEffectShader, TESEffectShader, EFSH) \
    code(BGSExplosion, BGSExplosion, EXPL) \
    code(BGSDebris, BGSDebris, DEBR) \
    code(TESImageSpace, TESImageSpace, IMGS) \
    code(TESImageSpaceModifier, TESImageSpaceModifier, IMAD) \
    code(BGSListForm, BGSListForm, FLST) \
    code(BGSPerk, BGSPerk, PERK) \
    code(BGSBodyPartData, BGSBodyPartData, BPTD) \
    code(BGSAddonNode, BGSAddonNode, ADDN) \
    code(ActorValueInfo, ActorValueInfo, AVIF) \
    code(BGSCameraShot, BGSCameraShot, CAMS) \
    code(BGSCameraPath, BGSCameraPath, CPTH) \
    code(BGSVoiceType, BGSVoiceType, VTYP) \
    code(BGSMaterialType, BGSMaterialType, MATT) \
    code(BGSImpactData, BGSImpactData, IPCT) \
    code(BGSImpactDataSet, BGSImpactDataSet, IPDS) \
    code(TESObjectARMA, TESObjectARMA, ARMA) \
    code(BGSEncounterZone, BGSEncounterZone, ECZN) \
    code(BGSLocation, BGSLocation, LCTN) \
    code(BGSMessage, BGSMessage, MESG) \
    code(BGSRagdoll, BGSRagdoll, RGDL) \
    code(BGSLightingTemplate, BGSLightingTemplate, LGTM) \
    code(BGSMusicType, BGSMusicType, MUSC) \
    code(BGSFootstep, BGSFootstep, FSTP) \
    code(BGSFootstepSet, BGSFootstepSet, FSTS) \
    code(BGSStoryManagerBranchNode, BGSStoryManagerBranchNode, SMBN) \
    code(BGSStoryManagerQuestNode, BGSStoryManagerQuestNode, SMQN) \
    code(BGSStoryManagerEventNode, BGSStoryManagerEventNode, SMEN) \
    code(BGSDialogueBranch, BGSDialogueBranch, DLBR) \
    code(BGSMusicTrackFormWrapper, BGSMusicTrackFormWrapper, MUST) \
    code(TESWordOfPower, TESWordOfPower, WOOP) \
    code(TESShout, TESShout, SHOU) \
    code(BGSEquipSlot, BGSEquipSlot, EQUP) \
    code(BGSRelationship, BGSRelationship, RELA) \
    code(BGSScene, BGSScene, SCEN) \
    code(BGSAssociationType, BGSAssociationType, ASTP) \
    code(BGSOutfit, BGSOutfit, OTFT) \
    code(BGSArtObject, BGSArtObject, ARTO) \
    code(BGSMaterialObject, BGSMaterialObject, MATO) \
    code(BGSMovementType, BGSMovementType, MOVT) \
    code(BGSSoundDescriptorForm, BGSSoundDescriptorForm, SNDR) \
    code(BGSDualCastData, BGSDualCastData, DUAL) \
    code(BGSSoundCategory, BGSSoundCategory, SNCT) \
    code(BGSSoundOutput, BGSSoundOutput, SOPM) \
    code(BGSCollisionLayer, BGSCollisionLayer, COLL) \
    code(BGSColorForm, BGSColorForm, CLFM) \
    code(BGSReverbParameters, BGSReverbParameters, REVB) \
    code(BGSModAttachmentMod, BGSMod::Attachment::Mod, OMOD) \
    code(BGSMaterialSwap, BGSMaterialSwap, MSWP) \

#define DEFINE_CLASS(name, type, id)  \
    class type;

FORM_TYPE_LIST(DEFINE_CLASS)

#undef DEFINE_CLASS

namespace DarkId::Papyrus::DebugServer
{
    template<typename... T>
    class FormVisitorBase
    {
    public:
        void Visit(TESForm* form, T... out)
        {
            switch (form->formType) {
#define DEFINE_SWITCH_CASE(name, type, id)  \
    case kFormType_##id: Visit##name((type*)form, out...); break; \

                FORM_TYPE_LIST(DEFINE_SWITCH_CASE)

#undef DEFINE_SWITCH_CASE
            default:
                DefaultVisit(form, out...);
            }
        }

        virtual void DefaultVisit(TESForm* form, T...) { }

#define DEFINE_VISIT(name, type, _)  \
    virtual void Visit##name(type* form, T... out) { DefaultVisit((TESForm*)form, out...); } \

        FORM_TYPE_LIST(DEFINE_VISIT)

#undef DEFINE_VISIT
    };
}