const DAY_PREF = 'd';
const ACT_PREF = 'a'

const LIST_DAY = ['Monday','Thusday','Wednesday','Thursday','Friday','Saturday','Sunday'];
// [formName [type, max limit, min limit,[inputNames],]]
const FORMS_LIST = [
  ['Network Settings',[['text','32','1',['SSID']],['text','32','8',['PWD']]]],
  ['Openweather Settings',[['text','32','1',['City']],['text','32','32',['Key']]]],
  ['Time Offset',[['number','23','-23',['Hour']], ['checkbox',0,,['Dst']]]]
];

const modal = window.document.getElementById('modal');


function getSetting()
{
  fetch('/data?', {
    method:'POST',
    mode: 'no-cors',
    body: null,
  })
  .then((r) => r.json())
  .then((r) => {
    for(const key in r){
      const value = r[key];
      if(key === 'Status'){
        const flags = Number(value);
        [...document.querySelectorAll('[type=checkbox]')].forEach((checkbox, i) =>{
            if (checkbox.id !== 'Dst') {
                checkbox.checked = flags&(1<<i);          
            }
          });
      } else if(key === 'Dst') {
          const input = document.getElementById(key);
          if(input) input.checked = (value == 1);
      } else {
        const input = document.getElementById(key);
        if(input)
          input.value = value;
      }
    }
   })
  .catch((e) => showModal(e));
}


function createForms()
{
  const containerForms = document.getElementById('settings_forms');
  FORMS_LIST.forEach((form_instance)=>{
    const [formName, inputList] = form_instance;
    const form = document.createElement('form');
    const container = document.createElement('div');
    const fieldset = document.createElement('fieldset');
    const legend = document.createElement('legend');
    legend.innerText = formName;
    fieldset.appendChild(form);
    fieldset.appendChild(legend);
    form.name = formName;
    form.action = '';
    const submit = document.createElement('input');
    submit.value = 'Submit';
    submit.type = 'submit';
    inputList.forEach((inputData)=>{
      const [type, maxLimit, minLimit, inputNames] = inputData;
      inputNames.forEach((inputName, i)=>{
        const label = document.createElement('label');
        label.innerText = inputName+' ';
          const input = document.createElement('input');
          input.type = type;
          input.id = inputName;
          input.name = inputName;
          if(type === 'text'){
            input.value = '';
            input.maxLength = maxLimit;
            input.minLength = minLimit;
            input.placeholder = 'Enter '+ inputName;
          } else if(type == 'checkbox' 
              && i >= maxLimit && inputName !== 'Dst'){
            input.disabled = true;
          } else if(type == 'number'){
            input.max = maxLimit;
            input.min = minLimit;
          }
          label.appendChild(input);
          form.appendChild(label);
        });
      });
      let hasNonCheckbox = false;
      inputList.forEach(inputData => {
          if (inputData[0] !== 'checkbox') hasNonCheckbox = true;
      });
      if(hasNonCheckbox){
        form.appendChild(submit);
      }
    container.appendChild(fieldset);
    containerForms.appendChild(container);
});
}

function getInfo()
{
  sendDataForm('info?');
}

function serverExit() 
{
  sendDataForm('close');
  document.getElementById('exit').classList.add('danger');
}


function showModal(str, success) 
{
  modal.innerText = str ? str : ':(';
  modal.style['background-color'] = success === true ? 'green':'red';
  modal.classList.add('show');
  const leftPos = (window.innerWidth - modal.offsetWidth) / 2;
  const topPos = (window.innerHeight - modal.offsetHeight) / 2 + window.scrollY;
  modal.style.right = leftPos + 'px';
  modal.style.top = topPos + 'px';
  setTimeout(() => {
    modal.classList.remove('show');
  },5000);
}


document.body.addEventListener('submit', (e) => {
  e.preventDefault();
  e.stopPropagation();
  sendData(e.target.name);
});


function sendData(formName)
{
  const js = {};
  let data = null;
  const childsList = document.forms[formName];
  if(childsList){
    for(const child of childsList){
      if(child.type === 'number' || child.type === 'text' || child.type === 'checkbox'){
        if(data == null) data = js;
        if(child.type === 'checkbox') {
          js[child.name] = child.checked ? 1 : 0;
        } else if (child.value) {
          js[child.name] = child.value;
        }
      }
    }
    if(data === js){
      data=JSON.stringify(js);
    }
    sendDataForm(formName, data);
  }
}


async function sendDataForm(path, data=""){
  let res = true;
  await fetch('/'+ path, {
    method:'POST',
    mode:'no-cors',
    body:data,
  }).then((r)=>{
      if(!r.ok)
        res=false; 
      return r.text()
    }).then((r) => showModal(r, res))
      .catch((e) => showModal(e, false));
}

createForms();

