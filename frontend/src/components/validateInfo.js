export default function validateInfo(values) {
    let errors = {}

    //name
    if(!values.Gname.trim()) {
        errors.Gname = "Name required"
    }

    //num guests
    if(!values.numGuests.trim()) {
        errors.numGuests = "Number of guests required"
    }

    //phone
    if(!values.phone.trim()) {
        errors.phone = "Phone number required"
    }

    return errors;
}