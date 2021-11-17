import React from 'react'
import { useState} from 'react';
import './Form.css'
import './Button.css'
import {v4 as uuid4} from 'uuid';
import ShowQueue from './ShowQueue';
import Swal from 'sweetalert2';

const mqtt = require('mqtt');
const day = new Date();
const timelimit = ['10:00','22:00'];
const today = day.getFullYear() +'-'+ (day.getMonth()+1)+'-'+day.getDate();

// credit func : https://stackoverflow.com/questions/29785294/check-if-current-time-is-between-two-given-times-in-javascript
function checktimeinterval()
{
    var startTime = timelimit[0];
    var endTime =  timelimit[1];

    let currentDate = day;   

    let startDate = new Date(currentDate.getTime());
    startDate.setHours(startTime.split(":")[0]);
    startDate.setMinutes(startTime.split(":")[1]);

    let endDate = new Date(currentDate.getTime());
    endDate.setHours(endTime.split(":")[0]);
    endDate.setMinutes(endTime.split(":")[1]);
    let valid = startDate <= currentDate && endDate >= currentDate;
    return valid;
}

//console.log(checktimeinterval());
let intime = checktimeinterval();
let Q = '';
let remaining = '';
let client_id='';
let info = localStorage.getItem('info') ? localStorage.getItem('info') : false;
if(!info || (JSON.parse(info).expire!== today)){
    localStorage.clear();
    client_id = uuid4().toString();
}
else{
    client_id = JSON.parse(info).ID;
    Q = JSON.parse(info).Q;
    remaining = JSON.parse(info).remain;
}
const pub_topic ='request';
let sub_topic ='respond/'+client_id;
let conoption={
    clean:true,
    protocolVersion:4,
    protocol:"ws",
    keepalive:60,
    clientId: client_id,
    will:{
        topic:pub_topic,
        payload:'Disconnect unexpected',
        qos:2,
        retain:false
        }
    };

var is_connected = false; // connect broker and backend
var client = mqtt.connect("ws://localhost:9001",conoption);
client.on('connect',()=>{is_connected = true;})
client.on('disconnect',()=>{is_connected=false;})
client.subscribe(sub_topic,{qos:2});
client.subscribe("recent_remaining",{qos:0});
client.on('message',(topic,msg)=>{
if(topic === sub_topic)
{        
    if(msg.toString() === 'DON' || msg.toString() ==='CBA')
    {
        let title = '';
        let text = ''
        let icon = '';
        if(msg.toString() === 'DON' )
        {    
           title = 'Thank you';
           text  = 'Your queue has been confirmed.';
           icon = 'success';
        }
        else if(msg.toString() === 'CBA' )
        {
            title = 'Sorry';
           text  = 'Your queue has been cancelled by us. We apologize for the inconvenience.';
           icon = 'error';
        }
        Swal.fire({
        title: title,
        text: text,
        icon: icon,
        showCloseButton: true,
        }).then(()=>{
            Q = '';
            remaining = '';
            client_id='';
            localStorage.clear();
            window.location.reload();
        })
    }
    else
    {
        let res =JSON.parse(msg.toString());
        Q = res.queue;
        remaining = res.remain;
        let store = {
                'ID':client_id,
                'Q':Q,
                'remain':remaining,
                'expire':today
        };
        localStorage.setItem('info',JSON.stringify(store));
        
    }
}
});
var errors = {};
function ValidateInfo(values){
        if (!values.Name.trim()) {
            errors.Name = 'Name required.';
        }
        else if((/[!@#$%^&*()_+\-=\[\]{};':"\\|,.<>\/?]+/.test(values.Name.trim()))  || (/\d/.test(values.Name.trim()))){
            errors.Name = 'Please enter a valid name that not contain any special characters or numbers.';
        }
    
        if(!values.tel.trim()){
            errors.tel = 'Telephone number requied.';
        }
        else if(!/^[0-9\b]+$/.test(values.tel.trim())){
            errors.tel = 'Please enter only number.'
        }
        else if(values.tel.trim().length !== 10){
            errors.tel = 'Please enter a valid phone number.'
        }
    
        if(!values.num_people.trim()){
            errors.num_people = 'Number of guests required.'
        }
        else if(isNaN(values.num_people.trim())){
            errors.num_people = 'Please enter a valid number of guests.'
        }
        else if(!(values.num_people.trim()>=1 && values.num_people.trim()<=10) )
        {
            errors.num_people = 'Limit guests at 1 - 10.'
        }
    
    }


const FormSignUp = () => {

   
        let [showremain,setShowR] = useState(false);
        let[values, setValues] = useState({
            Name: '',
            tel: '',
            num_people: '',
        });
        
        let [errText, setErrors] = useState({});
        
        let[state, setState] = useState({
            buttons: localStorage.getItem('info') ? true : false,
            inputs: localStorage.getItem('info') ? true : false,
        });
        
        const handleCancel = () => {
            Swal.fire({
                title: 'Are you sure you want to cancel?',
                text: "You won't be able to revert this!",
                icon: 'warning',
                showCancelButton: true,
                confirmButtonColor: '#3085d6',
                cancelButtonColor: '#d33',
                confirmButtonText: 'Confirm'
            }).then((result) => {
                if (result.isConfirmed) {
                    setShowR(false);
                    setState(false);
                    //console.log('Cancel');
                    let cancel_req = 'C:'+client_id;
                    if(is_connected)
                    {
                        client.publish(pub_topic,cancel_req,{qos:2},()=>{
                        localStorage.clear();
                        window.location.reload();
                    });
                    } 
                }
            })
        }
        
        
        const handleChange = (e) => {
            const { name, value } = e.target;
            setValues({
                ...values,
                [name]: value
            });
        };
        
        const handleSubmit = (e) => {
            if(!intime)
            {
                Swal.fire({
                    title: 'Sorry',
                    text: "Please come back later. Opening hour is 10.00 - 22.00 .",
                    icon: 'warning',
                    confirmButtonColor: '#3085d6',
                    showCloseButton: true,
                })
                return;
            }else if(!(is_connected))
            {
                Swal.fire({
                    title: 'Sorry',
                    text: "There is some problem with connection, please try again later.",
                    icon: 'warning',
                    showCloseButton: true,});
                return;
            }
            errors={};
            ValidateInfo(values);
        
            e.preventDefault();
            
            Swal.fire({
                    title: 'Are you sure you want to confirm?',
                    text: "You won't be able to revert this!",
                    icon: 'warning',
                    showCancelButton: true,
                    confirmButtonColor: '#3085d6',
                    cancelButtonColor: '#d33',
                    confirmButtonText: 'Confirm'
                }).then((result) => {
                    //console.log(errors);
                if (result.isConfirmed && !errors.Name && !errors.num_people && !errors.tel) {
                    setErrors({});
                    setShowR(false);
                    
                    const json_test={
                    ...values,
                    c_id:client_id,
                    };
                
                    if(is_connected)
                    {
                        client.publish(pub_topic,JSON.stringify(json_test),{qos:2});
                    }
                    setState({...state, buttons: true, inputs: true});
                    Swal.fire({
                        title:'Success!',
                        text:'Your queue has been confirm.',
                        icon:'success',
                        showCloseButton: true,
                    })
                  
                }
                else if(result.isDismissed)
                {
                    return;
                }
                else{
                    Swal.fire({
                        title: 'Oops...',
                        text: "Please check your form for errors and submit form again.",
                        icon: 'question',
                        showCloseButton: true,
                    })
                    setErrors(errors);
                }
            })
        };
        
    const handleShow =()=>
    {
        if(!intime)
            {
                Swal.fire({
                    title: 'Sorry',
                    text: "Please come back later. Opening hour is 10.00 - 22.00 .",
                    icon: 'warning',
                    confirmButtonColor: '#3085d6',
                    confirmButtonText: 'OK'
                })
                return;
            }
            else if(!(is_connected))
            {
                Swal.fire({
                    title: 'Sorry',
                    text: "There is some problem with connection, please try again later.",
                    icon: 'warning',
                    showCloseButton: true,});
                return;
            }
        setShowR(true);
    }

return (
    <div>
         <ShowQueue SR = {showremain} C = {client}/>
        <div className = 'form-container'>
        <div className="form-content">
            <form className="form" onSubmit={handleSubmit} id="form">
                <h1>
                    Reserve Table
                </h1>
                <div className="form-inputs-contents">
                    <div className="form-inputs">
                        <label htmlFor="Gname" className="form-label">
                            Name
                        </label>
                        <input 
                            id='Name'
                            type="text" 
                            name='Name' 
                            maxLength="100"
                            className="form-input"
                            placeholder="Enter your name"
                            value={values.Name}
                            onChange={handleChange}
                            disabled={state.inputs}
                            required
                        />
                        {errText.Name && <p>{errText.Name}</p>}
                    </div>
                    <br/>
                    <div className="form-inputs">
                        <label htmlFor="num_people" className="form-label">
                            Number of Guests
                        </label>
                        <input 
                            id='num_people'
                            type="number"
                            min="1"
                            max="10"
                            name='num_people' 
                            className="form-input"
                            value={values.num_people}
                            onChange={handleChange}
                            disabled={state.inputs }
                        />
                        {errText.num_people && <p>{errText.num_people}</p>}
                    </div>
                    <br/>
                    <div className="form-inputs">
                        <label htmlFor="phone" className="form-label">
                            Phone
                        </label>
                        <input 
                            id='tel'
                            type="tel" 
                            name='tel' 
                            className="form-input"
                            placeholder="Enter your phone number"
                            value={values.tel}
                            onChange={handleChange}
                            disabled={state.inputs } 
                        />
                        {errText.tel && <p>{errText.tel}</p>}
                    </div>
                    <br/>
                    <div className="form-btn">
                        {state.buttons ?
                            null
                            :
                            <button className="form-submit-btn" type="button" form="form" onClick={handleSubmit}>
                                Submit
                            </button>
                        }
                        {state.buttons ?
                            null
                            :
                            <button className="form-queue-btn" type="button" onClick ={handleShow}>
                                Show Remaining
                            </button>
                        }
                        {state.buttons ? 
                            <button className="form-cancel-btn" type="button" onClick={handleCancel}>
                                Cancel
                            </button>
                            :
                            null
                        }
                    </div> 
                </div>
            </form>
        </div>
        
    </div>
    </div>
    )
}

export default FormSignUp
