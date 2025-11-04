import { useLocation, useNavigate } from 'react-router-dom'
import Logo from '../components/Logo'
import BackButton from '../components/BackButton'

function DecodeResult() {
  const location = useLocation()
  const navigate = useNavigate()
  const { fileUrl, fileName, carrierName } = location.state || {}

  if (!fileUrl) {
    navigate('/')
    return null
  }

  const handleSave = () => {
    const a = document.createElement('a')
    a.href = fileUrl
    a.download = fileName || 'decoded.txt'
    document.body.appendChild(a)
    a.click()
    document.body.removeChild(a)
  }

  return (
    <div className="container">
      <BackButton />
      <Logo />
      
      <h2 className="title">Decoded Files</h2>
      
      <div className="result-files">
        <div className="file-display">{carrierName || 'Carrier.*ext'}</div>
        <div className="file-display">{fileName || 'Secret.*ext'}</div>
      </div>

      <button className="button primary" onClick={handleSave}>
        Save
      </button>
    </div>
  )
}

export default DecodeResult
