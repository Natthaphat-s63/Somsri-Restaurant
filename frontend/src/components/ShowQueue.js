import React,{useState} from 'react'
import './ShowQueue.css'

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

export default ShowQueue
