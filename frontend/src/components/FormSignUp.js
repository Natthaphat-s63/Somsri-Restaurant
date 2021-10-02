import React from 'react'
import { useState } from 'react';
import './Form.css'

const FormSignUp = () => {

    let[name, setName] = useState('');
    let[numPeople, setNumPeople] = useState('');
    let[tel, setTel] = useState('');

    let handleSubmit = (e) => {
        const info = {
            Name: name,
            num_people: numPeople,
            Tel: tel
        };
        console.log(JSON.stringify(info))
    }

    return (
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
                            id='Gname'
                            type="text" 
                            name='Gname' 
                            className="form-input"
                            placeholder="Enter your name"
                            value={name}
                            onChange={(e) => setName(e.target.value)}
                        />
                    </div>
                    <div className="form-inputs">
                        <label htmlFor="numGuests" className="form-label">
                            Number of Guests
                        </label>
                        <input 
                            id='numGuests'
                            type="number"
                            min="1"
                            max="20"
                            name='numGuests' 
                            className="form-input"
                            value={numPeople}
                            onChange={(e) => setNumPeople(e.target.value)}
                        />
                    </div>
                    <div className="form-inputs">
                        <label htmlFor="phone" className="form-label">
                            Phone
                        </label>
                        <input 
                            id='phone'
                            type="tel" 
                            name='phone' 
                            className="form-input"
                            placeholder="Enter your phone number"
                            pattern="[0-9]{10}"
                            value={tel}
                            onChange={(e) => setTel(e.target.value)}
                        />
                    </div>
                    <div className="form-btn">
                        <button className="form-input-btn" type="submit">
                            Submit
                        </button>
                        <button className="form-queue-btn" type="button">
                            Show Queue
                        </button>
                        <button className="form-cancel-btn" type="reset">
                            Cancel
                        </button>
                    </div>
                </div>
            </form>
        </div>
    )
}

export default FormSignUp
