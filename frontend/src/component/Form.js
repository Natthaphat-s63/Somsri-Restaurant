import React from "react";
import { useState } from "react";
const mqtt = require('mqtt');
const topic ='test';
const conoption={
    clean:true,
    protocolVersion:4,
    protocol:"ws",
    keepalive:60,
    will:{
        topic:topic,
        payload:'Disconnect unexpected',
        qos:2,
        retain:false
    }
};
var client = mqtt.connect("ws://localhost:9001",conoption);;
var is_connected = true;
const Form = () =>{

    const[name, setName] = useState('');
    const[reserveT, setReserveT] = useState('');
    const[numP, setNumP] = useState('');
    const[tel, setTel] = useState('');

    const handleSubmit = (e) => {
        e.preventDefault();
        const json_test={
            Name:name,
            tel:tel,
            num_people:numP,
            time_arrive:reserveT,
        };
        if(is_connected)
        {
            client.publish(topic,JSON.stringify(json_test));
            console.log(JSON.stringify(json_test));
            console.log('sending');
        }
    }
    const handleConn = () => {
        if(!is_connected)
        {
            client = mqtt.connect("ws://localhost:9001",{clean:true,protocolVersion:4,protocol:"ws"});
            console.log('Connect to server.');
            is_connected = true;
        }
        else
        {
            console.log('Already connected.');
        }
    }
    const handleDisConn = () => {
        client.end(true);
        is_connected =false;
        console.log('Disconnect');
        
    }

    return(
        <div>
            <h3>Information</h3>
            <form onSubmit={handleSubmit}>
                <div>
                    <br/>
                    <label htmlFor="Name">Name</label>
                    <br/>
                    <input 
                    type="text" 
                    name="Name" 
                    id="Name" 
                    placeholder="Name" 
                    required
                    value={name}
                    onChange={(e) => setName(e.target.value)}
                    />
                </div>
                <div>
                    <label htmlFor="ReserveTime">Reserve Time</label>
                    <br/>
                    <input 
                    type="time" 
                    name="ReserveTime"
                    min="10:00" 
                    max="21:00"
                    id="ReserveTime" 
                    required
                    value={reserveT}
                    onChange={(e) => setReserveT(e.target.value)}
                    />
                    <small> Open hours are 10:00 to 21:00</small>
                </div>
                <div>
                    <label htmlFor="numberP">Number of People</label>
                    <br/>
                    <input 
                    type="number"
                    min="1"
                    max="10"
                    name="NumP" 
                    id="NumP" 
                    placeholder="1-10" 
                    required
                    value={numP}
                    onChange={(e) => setNumP(e.target.value)}
                    />
                    <small> Numbers people are 1 to 10</small>
                </div>
                <div>
                    <label htmlFor="Phone">Telephone Number</label>
                    <br/>
                    <input 
                    type="tel"
                    name="Phone" 
                    id="Phone"
                    placeholder="XXX-XXX-XXXX"
                    pattern="[0-9]{10}"
                    required
                    value={tel}
                    onChange={(e) => setTel(e.target.value)}
                    />
                </div>
                <button type="Submit">Submit</button>
                <button onClick={handleConn}>Connect</button>
                <button onClick={handleDisConn}>Disconnect</button>
            </form>
        </div>
    )
}

export default Form;