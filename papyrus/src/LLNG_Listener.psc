Scriptname LLNG_Listener extends Quest

; SexLab P+ reports each actor's orgasm with the Papyrus-only mod event "SexlabOrgasmSeparate"
; (Form actor, int thread id); SKSE plugins cannot see it. This quest asks P+ who received it
; and hands every pair to LiquidLayersNG.dll, which does all the rest.

int Property KindVaginal = 1 AutoReadOnly
int Property KindAnal = 2 AutoReadOnly
int Property KindOral = 3 AutoReadOnly
int Property KindFacial = 4 AutoReadOnly

Event OnInit()
	RegisterForModEvent("SexlabOrgasmSeparate", "OnSexLabOrgasm")
	RegisterForModEvent("BiS_WashActorFinish", "OnBathe")
EndEvent

; Bathing in Skyrim (Renewed): sent when an actor finished washing
Event OnBathe(Form akActor, Form akWashProp, Bool abUsingSoap)
	Actor who = akActor as Actor
	If (who)
		LiquidLayersNG.Wash(who)
	EndIf
EndEvent

Event OnSexLabOrgasm(Form akActor, int aiThread)
	Actor giver = akActor as Actor
	If (!giver)
		Return
	EndIf
	SexLabFramework sl = Game.GetFormFromFile(0x000D62, "SexLab.esm") as SexLabFramework
	SexLabThread thread = None
	If (sl)
		thread = sl.GetThread(aiThread)
	EndIf
	If (!thread)
		Return
	EndIf
	If (thread.IsInteractionRegistered())
		; P+ interaction ids: aFacial 13, pOral 18, pDeepthroat 20, aVaginal 23, aAnal 25
		int sent = Send(giver, thread.GetPartnersByInteractionType(giver, 23), KindVaginal)
		sent += Send(giver, thread.GetPartnersByInteractionType(giver, 25), KindAnal)
		sent += Send(giver, thread.GetPartnersByInteractionType(giver, 20), KindOral)
		If (sent == 0)
			sent += Send(giver, thread.GetPartnersByInteractionType(giver, 18), KindOral)
		EndIf
		If (sent == 0)
			Send(giver, thread.GetPartnersByInteractionType(giver, 13), KindFacial)
		EndIf
		Return
	EndIf
	; No collision data: fall back to the scene tags, partners = the other positions without a penis.
	int sex = SexLabRegistry.GetSex(giver, False)
	If (sex == 1 || sex == 4)
		Return
	EndIf
	int kind = 0
	If (thread.HasSceneTag("Vaginal"))
		kind = KindVaginal
	ElseIf (thread.HasSceneTag("Anal"))
		kind = KindAnal
	ElseIf (thread.HasSceneTag("Oral"))
		kind = KindOral
	EndIf
	If (kind == 0)
		Return
	EndIf
	Actor[] positions = thread.GetPositions()
	int i = 0
	While (i < positions.Length)
		Actor other = positions[i]
		If (other && other != giver)
			int otherSex = SexLabRegistry.GetSex(other, False)
			If (otherSex == 1 || otherSex == 4)
				LiquidLayersNG.Climax(giver, other, kind)
			EndIf
		EndIf
		i += 1
	EndWhile
EndEvent

int Function Send(Actor akGiver, Actor[] akPartners, int aiKind)
	int count = 0
	int i = 0
	While (i < akPartners.Length)
		If (akPartners[i] && akPartners[i] != akGiver)
			LiquidLayersNG.Climax(akGiver, akPartners[i], aiKind)
			count += 1
		EndIf
		i += 1
	EndWhile
	Return count
EndFunction
