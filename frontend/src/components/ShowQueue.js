import React from 'react'
import './ShowQueue.css'

function ShowQueue() {
    return (
        <div className="queue-container">
            <h1 className="Title">QUEUE</h1>
            <div className="queue-content">  
                <div className="Qnumber">
                    <h3 className="Qnumber_title">
                        Q number
                    </h3>
                    <h2>
                        A100
                    </h2>
                    
                </div>
                <div className="Qwaiting">
                        <h3>
                            Waiting
                        </h3>
                        <h2>
                            12
                        </h2>
                </div>
            </div>
        </div>
    )
}

export default ShowQueue
