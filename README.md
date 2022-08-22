
#MoHSES Simple Assessment Module

This is a module built to evaluate simple assessment checklists, both sequential and non-sequential.

An assessment list can be created as shown below:
```xml
<AssessmentList type="sequential">
   <Assessment event="LOOK_LISTEN_FEEL_CHEST" description="Observed breathing" automated="false" />
   <Assessment event="CHECK_GCS" description="LOC checked" automated="false" />
   <Assessment event="CONNECT_PULSE_OX" description="Pulse ox applied" automated="true" />
   <Assessment event="TAKE_PULSE" description="Pulse checked" />
   <Assessment event="CONNECT_PULSE_OX" description="Pulse checked" automated="false" />
   <Assessment event="HEAD_FEEL_SKIN" description="Skin checked" automated="false" />
   <Assessment event="SUPINE_POSITION" description="Patient positioned properly" automated="false" />
   <Assessment event="CHECK_MOUTH" description="Airway checked" automated="false" />
   <Assessment event="OPEN_AIRWAY" description="Opened airway" automated="false" />
   <Assessment event="BVM_ON" description="BVM applied" automated="true" />
   <Assessment event="O2_GIVEN" description="Supplemental O2 applied" automated="true" />
   <Assessment event="CHECK_PUPILS" description="Pupils checked" automated="false" />
   <Assessment event="CHECK_HEAD" description="Checked for trauma" automated="false" />
   <Assessment event="CHECK_IV_DRUG_USE" description="Checked for IV drug use" automated="false" />
   <Assessment event="NALOXONE_GIVEN" description="Naloxone administered" automated="true" />
   <Assessment event="CONTINUE_AIRWAY_SUPPORT" description="Continued airway support" automated="false" />
</AssessmentList>
```

This is included in the capability configuration section of the MoHSES Scenario File format.

For each event record of a specific type that comes in a MoHSES Assessment will be generated, indicating one of the following:

* Success
* Commission Error (list is sequential but event arrived out of order)
* Omission Error (simulation has ended, event did not arrive)
