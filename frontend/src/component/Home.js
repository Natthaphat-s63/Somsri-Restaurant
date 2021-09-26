import React from "react";
import { useState } from "react";

const Home = () => {
    const [name, setName] = useState(null);

    function getData(val){
        setName(val.target.value);
    }


    const handleClick = () => {
        console.log('Hello, ' + name);
    }

    const handleConn = () => {
        console.log('Connect to server.');
    }

    const handleDisConn = () => {
        console.log('Disconnect to server.');
    }

    return(
        <div className="home">
            <h2>Test box</h2>
            <input type="text" onChange={getData}/>
            <button onClick={handleClick}>Send</button>
            <button onClick={handleConn}>Connect</button>
            <button onClick={handleDisConn}>Disconnect</button>
        </div>
    );
}

export default Home;