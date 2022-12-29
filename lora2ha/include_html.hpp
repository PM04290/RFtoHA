const char html_header[] PROGMEM = R"rawliteral(
<div class="row col-md-12 pb-2">
  <div class="form-group  col-sm-1">
    <div class="input-group">
      <span class="input-group-text">UID</span>
      <input type="text" class="form-control" name="uniqueid" value="%CNFUID%" readonly>
    </div>
  </div>
  <div class="form-group  col-sm-1">
    <div class="input-group">
      <span class="input-group-text">Ver. Maj.</span>
      <input type="number" class="form-control" name="version_major" value="%CNFVMAJ%" readonly>
    </div>
  </div>
  <div class="form-group  col-sm-1">
    <div class="input-group">
      <span class="input-group-text">Ver. Min.</span>
      <input type="number" class="form-control" name="version_minor" value="%CNFVMIN%" readonly>
    </div>
  </div>
</div>
<div id="conf_dev" class="row">chargement...</div>
<div class="col-md-12 mt-2">
  <div class="input-group justify-content-end">
    <button type="button" class="btn btn-info bi-plus-circle" id="newdev" onclick="adddev(this)">&nbsp;Ajout module</button>
  </div>
</div>
)rawliteral";

const char html_device[] PROGMEM = R"rawliteral(
<div class="col-md-1 mt-2">
  <div class="input-group">
    <span class="input-group-text">@</span>
    <input type="text" class="form-control" name="dev_#D#_address" value="%CNFADDRESS%">
  </div>
</div>
<div class="col-md-11 mt-2" id="develt_#D#">
  <div class="row pb-1">
    <div class="col-sm-2">
      <div class="input-group">
        <span class="input-group-text">Nom</span>
        <input type="text" class="form-control" name="dev_#D#_name" value="%CNFNAME%">
      </div>
    </div>
  </div>
 %GENCHILDS%
  <div class="col-md-12 mt-2">
    <div class="input-group justify-content-end">
      <button type="button" class="btn btn-info bi-plus-circle" id="newchild_#D#" onclick="addchild(this)">&nbsp;Ajout capteur</button>
    </div>
  </div>
</div>
)rawliteral";

const char html_child[] PROGMEM = R"rawliteral(
<div class="row pt-1">
  <div class="col-sm-1">
    <div class="input-group">
      <span class="input-group-text bi-box-arrow-right"></span>
      <input type="number" class="form-control" name="dev_#D#_childs_#C#_id" value="%CNFC_ID%">
    </div>
  </div>
  <div class="col-sm-2">
    <div class="input-group">
      <span class="input-group-text">Label</span>
      <input type="text" class="form-control" name="dev_#D#_childs_#C#_label" value="%CNFC_LABEL%">
    </div>
  </div>
  <div class="col-sm-2">
    <div class="input-group">
      <label class="input-group-text">Sensor type</label>
      <select class="form-select" name="dev_#D#_childs_#C#_sensortype" >
        <option value="0" %CNFC_S0%>Binary sensor</option>
        <option value="1" %CNFC_S1%>Numeric sensor</option>
        <option value="2" %CNFC_S2%>Switch</option>
        <option value="3" %CNFC_S3%>Light</option>
        <option value="4" %CNFC_S4%>Cover</option>
        <option value="5" %CNFC_S5% disabled>Fan</option>
        <option value="6" %CNFC_S6% disabled>HVac</option>
        <option value="7" %CNFC_S7% disabled>Select</option>
        <option value="8" %CNFC_S8%>Trigger</option>
        <option value="9" %CNFC_S9% disabled>Custom</option>
        <option value="10" %CNFC_S10%>Tag</option>
      </select>
    </div>
  </div>
  <div class="col-sm-2">
    <div class="input-group">
      <label class="input-group-text">Data type</label>
      <select class="form-select" name="dev_#D#_childs_#C#_datatype" >
        <option value="0" %CNFC_D0%>Boolean</option>
        <option value="1" %CNFC_D1%>Int</option>
        <option value="2" %CNFC_D2%>Float</option>
        <option value="3" %CNFC_D3%>Text</option>
        <option value="4" %CNFC_D4%>Tag</option>
        <option value="5" %CNFC_D5%>Raw</option>
      </select>
    </div>
  </div>
</div>
)rawliteral";
