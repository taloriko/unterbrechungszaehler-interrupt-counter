#pragma once

#include <Arduino.h>

// Einheitliche Benennung der drei Ringspeicher in Geraeteansicht und Export.
static const char WEB_UI_STORAGE[] PROGMEM = R"HTML(
<style>
.ringPurpose{margin:0 0 12px;line-height:1.45;color:var(--text)}
.ringPurpose strong{font-weight:700}
</style>
<script>
(function(){'use strict';
const $=id=>document.getElementById(id);
const text=(de,en)=>document.documentElement.lang==='en'?en:de;

function setPreviousLabel(valueId,label){
  const value=$(valueId);
  if(value&&value.previousElementSibling)value.previousElementSibling.textContent=label;
}

function setMeterLabel(id,label){
  const value=$(id+'Text');
  const head=value&&value.closest('.usageHead');
  const labelNode=head&&head.querySelector('span');
  if(labelNode)labelNode.textContent=label;
}

function configureExportCard(href,title,purpose,buttonText){
  const link=document.querySelector('#export a[href="'+href+'"]');
  const card=link&&link.closest('.card');
  if(!card)return;

  const heading=card.querySelector('h2');
  if(heading)heading.innerHTML='💾 '+title;

  let purposeNode=card.querySelector('.ringPurpose');
  const oldParagraph=card.querySelector('p');
  if(!purposeNode){
    purposeNode=document.createElement('p');
    purposeNode.className='ringPurpose';
    if(oldParagraph)oldParagraph.replaceWith(purposeNode);
    else if(heading)heading.insertAdjacentElement('afterend',purposeNode);
  }
  purposeNode.innerHTML='<strong>'+text('Zweck:','Purpose:')+'</strong> '+purpose;
  link.textContent=buttonText;
}

function normalizeStorageUi(){
  const normal=text('Normal-Ringspeicher','Normal ring buffer');
  const archive=text('Langzeit-Ringspeicher','Long-term ring buffer');
  const autark=text('Autark-Ringspeicher','Standalone ring buffer');

  setPreviousLabel('devRecent',normal);
  setPreviousLabel('devArchive',archive);
  setPreviousLabel('devAutark',autark);
  setMeterLabel('meterRecent',normal);
  setMeterLabel('meterArchive',archive);
  setMeterLabel('meterAutark',autark);

  configureExportCard(
    '/export.csv',
    normal,
    text('Schneller Ringspeicher für die letzten Unterbrechungen und die Webansicht.',
         'Fast ring buffer for recent interruptions and the web interface.'),
    text('Normal-Ringspeicher als CSV','Download normal ring buffer CSV')
  );

  configureExportCard(
    '/autark.csv',
    autark,
    text('Speichert Autark-Sessions, Pulse, relative Zeiten und Zeitanker.',
         'Stores standalone sessions, pulses, relative times and time anchors.'),
    text('Autark-Ringspeicher als CSV','Download standalone ring buffer CSV')
  );

  configureExportCard(
    '/archive.csv',
    archive,
    text('Langzeitbasis für Statistik, Heatmaps und den vollständigen Archivexport.',
         'Long-term source for statistics, heatmaps and the complete archive export.'),
    text('Langzeit-Ringspeicher als CSV','Download long-term ring buffer CSV')
  );
}

normalizeStorageUi();
setTimeout(normalizeStorageUi,0);
const language=$('languageSelect');
if(language)language.addEventListener('change',()=>setTimeout(normalizeStorageUi,20));
})();
</script>
)HTML";
