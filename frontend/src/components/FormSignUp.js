import React from 'react'
import { useState,useEffect} from 'react';
import './Form.css'
import './ShowQueue.css'
import './Button.css'
import {v4 as uuid4} from 'uuid';

    const mqtt = require('mqtt');
    const day = new Date();
    const today = day.getFullYear() +'-'+ (day.getMonth()+1)+'-'+day.getDate();
    let Q = '';
    let remaining = '';
    let client_id='';
    let R = '';
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
    const pub_topic ='test';
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
    var submitted = false;
    var showR_state = false;
    var is_connected = false;
    var client = mqtt.connect("ws://localhost:9001",conoption);
    var status='';
    client.on('connect',()=>{is_connected = true;})
    client.on('disconnect',()=>{is_connected=false;})
    client.subscribe(sub_topic,{qos:2});
    client.subscribe("showR",{qos:0});
    client.on('message',(topic,msg)=>{
        console.log("respond = "+msg.toString());
    if(topic === sub_topic)
    {        
        if(msg.toString() === 'DON' || msg.toString() ==='CBA' || msg.toString() === 'CBC')
        {
            status = msg.toString();
            Q = '';
            remaining = '';
            client_id='';
            localStorage.clear();
            window.location.reload();
        }
        else
        {
            let res =JSON.parse(msg.toString());
            Q = res.queue;
            remaining = res.remain;
            console.log(Q);
            let store = {
                    'ID':client_id,
                    'Q':Q,
                    'remain':remaining,
                    'expire':today
            };
            localStorage.setItem('info',JSON.stringify(store));
        }
    }
    else if(topic === 'showR')
    {
        let res =JSON.parse(msg.toString());
        R = res.showR;
        //console.log(R);
    }
    });

function ShowQueue() {
        let[disp_q, setdisp_q] = useState('--');
        let[disp_r, setdisp_r] = useState('--');
        setInterval(() => {
            if(localStorage.getItem('info'))
            {
                let info = JSON.parse(localStorage.getItem('info'));
                setdisp_q(info.Q);
                setdisp_r(info.remain);
            }   
            if(showR_state)
            {
                setdisp_r(R);
            }   
        }, 100);
        return (
            <div className="queue-container">
                <h1 className="Title">QUEUE</h1>
                <div className="queue-content">  
                    <div className="Qnumber">
                        <h3 className="Qnumber_title">
                            Q number
                        </h3>
                        <h2>
                            {disp_q}
                        </h2>
                        
                    </div>
                    <div className="Qwaiting">
                            <h3>
                                Waiting
                            </h3>
                            <h2>
                            {disp_r}
                            </h2>
                    </div>
                </div>
            </div>
        )
}



const FormSignUp = () => {
    
    let[values, setValues] = useState({
        Name: '',
        tel: '',
        num_people: '',
    });
    let[state, setState] = useState({
        buttons: localStorage.getItem('info') ? true : false,
        inputs: localStorage.getItem('info') ? true : false,
    });
    const handleCancel = (e) => {
        alert('Do you want to cancel?');
        showR_state = false;
        setState(false);
        localStorage.clear();
        console.log('Cancel');
        window.location.reload();
    }
    const handleChange = (e) => {
        const { name, value } = e.target;
        setValues({
            ...values,
            [name]: value
        });
    };
    const handleSubmit = (e) => {
        showR_state = false;
        submitted = true;
        e.preventDefault();
        const json_test={
            ...values,
            c_id:client_id,
        };
        if(is_connected)
        {
            client.publish(pub_topic,JSON.stringify(json_test),{qos:2});
            console.log(JSON.stringify(json_test));
        }
        setState({...state, buttons: true, inputs: true});
        console.log('Submit');
        console.log(state);
    }
    const handleShow =()=>
    {
        showR_state =true;
    }

    
return (
    <div>
        <ShowQueue/>
        <div className = 'form-container'>
        <div className="form-content">
            <form className="form" onSubmit={handleSubmit}>
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
                            className="form-input"
                            placeholder="Enter your name"
                            value={values.Name}
                            onChange={handleChange}
                            disabled={state.inputs}
                            required 
                        />
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
                            disabled={state.inputs}
                            required
                        />
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
                            pattern="[0-9]{10}"
                            value={values.tel}
                            onChange={handleChange}
                            disabled={state.inputs}
                            required
                        />
                    </div>
                    <br/>
                    <div className="form-btn">
                        <button className="form-submit-btn" type="submit" disabled={state.buttons}>
                            Submit
                        </button>
                        <button className="form-queue-btn" type="button" disabled={!submitted} onClick={handleShow} >
                            Show Queue
                        </button>
                        <button className="form-cancel-btn" type="button" disabled={!state.buttons} onClick={handleCancel}>
                            Cancel
                        </button>
                    </div> 
                </div>
            </form>
        </div>
    </div>
    </div>
    )
};
export default FormSignUp
