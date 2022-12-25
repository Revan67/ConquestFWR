//lodctrl.cpp
#define LOD_CONTROL_CLASS_ID	Class_ID(0xbbe961a8,0xa0ee7b7f)

class LODCtrl : public StdControl {
	public:
		float min, max, bmin, bmax;
		WORD grpID;
		int order;
		BOOL viewport, highest;
#if 0
		LODCtrl();

		// Animatable methods		
		void DeleteThis() {delete this;}		
		int IsKeyable() {return 0;}		
		Class_ID ClassID() {return LOD_CONTROL_CLASS_ID;} 
		SClass_ID SuperClassID() {return CTRL_FLOAT_CLASS_ID;}
		//void GetClassName(TSTR& s) {s = LOD_CONTROL_CNAME;}
		BOOL CanCopyAnim() {return FALSE;}
		BOOL CanMakeUnique() {return FALSE;}		

		// Reference methods
		RefResult NotifyRefChanged(Interval, RefTargetHandle, PartID&, RefMessage) {return REF_SUCCEED;}
		IOResult Save(ISave *isave);
		IOResult Load(ILoad *iload);
		RefTargetHandle Clone(RemapDir &remap = NoRemap());

		// Control methods				
		void Copy(Control *from) {}
		BOOL IsLeaf() {return TRUE;}
		void CommitValue(TimeValue t) {}
		void RestoreValue(TimeValue t) {}
		BOOL IsReplaceable() {return FALSE;}
		BOOL CanInstanceController() {return FALSE;}

		// StdControl methods
		void GetValueLocalTime(TimeValue t, void *val, Interval &valid, GetSetMethod method=CTRL_ABSOLUTE)
			{*((float*)val) = 1.0f;}
		void SetValueLocalTime(TimeValue t, void *val, int commit, GetSetMethod method) {}
		void Extrapolate(Interval range,TimeValue t,void *val,Interval &valid,int type)
			{*((float*)val) = 1.0f;}
		void *CreateTempValue() {return new float;}
		void DeleteTempValue(void *val) {delete (float*)val;}
		void ApplyValue(void *val, void *delta) {*((float*)val) += *((float*)delta);}
		void MultiplyValue(void *val, float m) {*((float*)val) *= m;}

		float EvalVisibility(TimeValue t,View &view,Box3 pbox,Interval &valid);
		BOOL VisibleInViewports();
#endif
	};