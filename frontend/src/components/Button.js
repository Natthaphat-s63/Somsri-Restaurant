import React from 'react'

function Button() {
    return (
        <div className="form-btn">
            <button className="form-submit-btn" type="submit">
                Submit
            </button>
            <button className="form-queue-btn" type="button">
                Show Queue
            </button>
            <button className="form-cancel-btn" type="button">
                Cancel
            </button>
        </div>
    )
}

export default Button
